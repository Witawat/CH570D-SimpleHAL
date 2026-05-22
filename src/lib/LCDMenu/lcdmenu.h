/**
 * @file lcdmenu.h
 * @brief LCD Menu Library สำหรับ CH57x — เมนูหลายระดับบน LCD1602/LCD2004 พร้อมปุ่ม 4 ปุ่ม
 * @version 1.0
 * @date 2026-05-04
 * 
 * @details
 * Library สำหรับสร้างเมนูบน LCD1602 I2C / LCD2004 I2C ควบคุมด้วยปุ่ม 4 ปุ่ม
 * (UP / DOWN / ENTER / BACK) แบบ page-based display
 * 
 * **คุณสมบัติ:**
 * - รองรับเมนูย่อยซ้อนกันได้หลายระดับ (sub-menu)
 * - Menu item 4 ประเภท: Sub-menu, Callback, Toggle (ON/OFF), Value (ปรับค่า)
 * - Edit mode สำหรับ Toggle และ Value items
 * - Page scrolling อัตโนมัติเมื่อ items เกินจำนวนแถวของ LCD
 * - Custom cursor character (ลูกศร `>`)
 * - ใช้ Button Library สำหรับ debounce input
 * - ใช้ LCD1602_I2C Library สำหรับแสดงผล
 * - Static memory pool (ไม่ใช้ malloc)
 * 
 * **Hardware Requirements:**
 * - LCD1602 หรือ LCD2004 แบบ I2C (PCF8574)
 * - ปุ่ม 4 ปุ่มต่อกับ GPIO (active LOW แนะนำ)
 * - I2C ต้อง initialized ก่อนเรียก LCDMenu_Init
 * - Timer ต้อง initialized ก่อนเรียก LCDMenu_Init (Button ต้องใช้ Get_CurrentMs)
 * 
 * @example
 * // ตัวอย่าง complete menu
 * #include "SimpleHAL.h"
 * #include "Lib/LCD1602_I2C/lcd1602_i2c.h"
 * #include "Lib/Button/Button.h"
 * #include "Lib/LCDMenu/lcdmenu.h"
 * 
 * LCD1602_Handle lcd;
 * Button_Instance btn_up, btn_down, btn_enter, btn_back;
 * LCDMenu_Handle menu;
 * 
 * // Toggle callback
 * void on_backlight_changed(void) {
 *     if (*(menu.current_item->bool_value)) {
 *         LCD_Backlight(&lcd, 1);
 *     } else {
 *         LCD_Backlight(&lcd, 0);
 *     }
 * }
 * 
 * int main(void) {
 *     SystemCoreClockUpdate();
 *     NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
 *     Timer_Init();
 *     I2C_SimpleInit(I2C_100KHZ, I2C_PINS_DEFAULT);
 * 
 *     LCD_Init(&lcd, 0x27, LCD_16x2);
 *     LCD_Backlight(&lcd, 1);
 * 
 *     Button_Init(&btn_up,    PC0, BUTTON_ACTIVE_LOW);
 *     Button_Init(&btn_down,  PC1, BUTTON_ACTIVE_LOW);
 *     Button_Init(&btn_enter, PC2, BUTTON_ACTIVE_LOW);
 *     Button_Init(&btn_back,  PC3, BUTTON_ACTIVE_LOW);
 * 
 *     LCDMenu_Init(&menu, &lcd, &btn_up, &btn_down, &btn_enter, &btn_back);
 *     LCDMenu_CreateRoot(&menu, "Main Menu");
 * 
 *     // เพิ่ม sub-menu
 *     LCDMenu_Item* settings = LCDMenu_AddSubMenu(&menu, menu.root, "Settings", NULL);
 *     LCDMenu_AddSubMenu(&menu, menu.root, "About", NULL);
 * 
 *     // เพิ่ม items ใน Settings
 *     static bool bl_on = true;
 *     LCDMenu_AddToggle(&menu, settings, "Backlight", &bl_on, on_backlight_changed);
 * 
 *     static int32_t contrast = 50;
 *     LCDMenu_AddValue(&menu, settings, "Contrast", &contrast, 0, 100, 5, NULL);
 * 
 *     LCDMenu_Draw(&menu);
 * 
 *     while (1) {
 *         Button_Update(&btn_up);
 *         Button_Update(&btn_down);
 *         Button_Update(&btn_enter);
 *         Button_Update(&btn_back);
 *         LCDMenu_Update(&menu);
 *         Delay_Ms(5);
 *     }
 * }
 * 
 * @author CH57x Library Team
 * @copyright MIT License
 */

#ifndef __LCDMENU_H
#define __LCDMENU_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../LCD1602_I2C/lcd1602_i2c.h"
#include "../Button/Button.h"
#include <stdint.h>
#include <stdbool.h>

/* ========== Configuration ========== */

/**
 * @brief จำนวน menu items สูงสุดใน pool
 * @note ปรับได้ตาม RAM/flash ที่มี ถ้ามีเมนูลึกมากหรือ items เยอะ ให้เพิ่มค่านี้
 */
#ifndef LCDMENU_MAX_ITEMS
#define LCDMENU_MAX_ITEMS    32
#endif

/**
 * @brief จำนวน children สูงสุดต่อ 1 parent menu
 */
#ifndef LCDMENU_MAX_CHILDREN
#define LCDMENU_MAX_CHILDREN 16
#endif

/* ========== Enumerations ========== */

/**
 * @brief ประเภทของ menu item
 */
typedef enum {
    LCDMENU_TYPE_SUBMENU  = 0,  /**< เมนูย่อย (มี children) */
    LCDMENU_TYPE_CALLBACK = 1,  /**< เรียก callback เมื่อกด ENTER */
    LCDMENU_TYPE_TOGGLE   = 2,  /**< เปิด/ปิด (bool) */
    LCDMENU_TYPE_VALUE    = 3   /**< ปรับค่าตัวเลข (int32_t) */
} LCDMenu_ItemType;

/* ========== Forward Declarations ========== */

typedef struct LCDMenu_Item LCDMenu_Item;
typedef struct LCDMenu_Handle LCDMenu_Handle;

/* ========== Structures ========== */

/**
 * @brief โครงสร้างข้อมูลสำหรับ 1 menu item
 */
struct LCDMenu_Item {
    const char* name;                   /**< ชื่อที่แสดงบนเมนู */
    LCDMenu_ItemType type;              /**< ประเภทของ item */
    
    /* Tree structure */
    LCDMenu_Item* parent;               /**< เมนูแม่ (NULL สำหรับ root) */
    LCDMenu_Item* children[LCDMENU_MAX_CHILDREN]; /**< ลูก (สำหรับ SUBMENU) */
    uint8_t child_count;                /**< จำนวนลูก */
    
    /* Callback */
    void (*callback)(void);             /**< เรียกเมื่อกด ENTER (CALLBACK type) หรือเมื่อค่าเปลี่ยน (TOGGLE/VALUE) */
    
    /* Value fields (สำหรับ TOGGLE และ VALUE) */
    bool*    bool_value;                /**< ตัวชี้ไปยังค่า bool (TOGGLE) */
    int32_t* int32_value;               /**< ตัวชี้ไปยังค่า int32_t (VALUE) */
    int32_t  value_min;                 /**< ค่าต่ำสุด (VALUE) */
    int32_t  value_max;                 /**< ค่าสูงสุด (VALUE) */
    int32_t  value_step;                /**< ขั้นการปรับ (VALUE) */
    
    /* Internal */
    uint8_t  pool_index;                /**< index ใน static pool */
    uint8_t  initialized;               /**< flag init */
};

/**
 * @brief โครงสร้างข้อมูลหลักของ LCDMenu
 */
struct LCDMenu_Handle {
    LCD1602_Handle*  lcd;               /**< ตัวชี้ไปยัง LCD handle */
    
    /* Button handles (4 ปุ่ม) */
    Button_Instance* btn_up;            /**< ปุ่มเลื่อนขึ้น */
    Button_Instance* btn_down;          /**< ปุ่มเลื่อนลง */
    Button_Instance* btn_enter;         /**< ปุ่มเลือก/ยืนยัน */
    Button_Instance* btn_back;          /**< ปุ่มย้อนกลับ */
    
    /* Menu tree */
    LCDMenu_Item* root;                 /**< root menu (ไม่แสดง, ใช้เป็น container) */
    LCDMenu_Item* current_menu;         /**< เมนูที่กำลังแสดงอยู่ */
    
    /* Cursor & display */
    int8_t   cursor_index;              /**< index ของ item ที่ cursor ชี้อยู่ (0-based) */
    int8_t   display_offset;            /**< offset ของ display window (ใช้เมื่อ items เกินจอ) */
    uint8_t  visible_rows;              /**< จำนวนแถวที่มองเห็นได้ (2 สำหรับ 16x2, 4 สำหรับ 20x4) */
    uint8_t  title_row;                 /**< แถวที่ใช้แสดง title (0 = row 0) */
    
    /* Edit mode */
    bool     edit_mode;                 /**< true = กำลังแก้ไขค่า (TOGGLE/VALUE) */
    int32_t  edit_value;               /**< ค่าชั่วคราวระหว่างแก้ไข (VALUE) */
    
    /* State */
    uint8_t  dirty;                     /**< true = ต้อง redraw */
    uint8_t  initialized;               /**< flag init */
    
    /* Custom character slot */
    uint8_t  cgram_cursor;              /**< CGRAM location ของ cursor character */
};

/* ========== Function Prototypes ========== */

/* === Initialization === */

/**
 * @brief เริ่มต้น LCDMenu
 * 
 * @param menu      ตัวชี้ไปยัง LCDMenu_Handle
 * @param lcd       ตัวชี้ไปยัง LCD1602_Handle ที่ initialized แล้ว
 * @param btn_up    ตัวชี้ไปยัง Button_Instance สำหรับปุ่ม UP
 * @param btn_down  ตัวชี้ไปยัง Button_Instance สำหรับปุ่ม DOWN
 * @param btn_enter ตัวชี้ไปยัง Button_Instance สำหรับปุ่ม ENTER
 * @param btn_back  ตัวชี้ไปยัง Button_Instance สำหรับปุ่ม BACK
 * 
 * @note ต้องเรียก LCD_Init(), Button_Init() สำหรับทุกปุ่ม และ Timer_Init() ก่อน
 * @note CGRAM location 0 จะถูกจองสำหรับ cursor character โดยอัตโนมัติ
 * 
 * @example
 * LCDMenu_Handle menu;
 * LCDMenu_Init(&menu, &lcd, &btn_up, &btn_down, &btn_enter, &btn_back);
 */
void LCDMenu_Init(LCDMenu_Handle* menu,
                  LCD1602_Handle* lcd,
                  Button_Instance* btn_up,
                  Button_Instance* btn_down,
                  Button_Instance* btn_enter,
                  Button_Instance* btn_back);

/* === Menu Tree Construction === */

/**
 * @brief สร้าง root menu (container หลัก)
 * 
 * @param menu  ตัวชี้ไปยัง LCDMenu_Handle
 * @param title ชื่อ title ที่แสดงบนบรรทัดแรก
 * 
 * @note ต้องเรียกก่อนเพิ่ม items อื่นๆ
 * @note root menu จะไม่ถูกแสดงผลเอง แต่ children จะถูกแสดงเมื่อเรียก LCDMenu_Draw
 * 
 * @example
 * LCDMenu_CreateRoot(&menu, "Main Menu");
 */
void LCDMenu_CreateRoot(LCDMenu_Handle* menu, const char* title);

/**
 * @brief เพิ่ม sub-menu item (มี children)
 * 
 * @param menu     ตัวชี้ไปยัง LCDMenu_Handle
 * @param parent   เมนูแม่ที่จะเพิ่ม item นี้เข้าไป
 * @param name     ชื่อที่แสดง
 * @param callback เรียกเมื่อเข้า sub-menu ครั้งแรก (NULL = ไม่ใช้)
 * @return ตัวชี้ไปยัง item ที่สร้าง (ใช้เป็น parent สำหรับ children ต่อไป)
 * 
 * @example
 * LCDMenu_Item* settings = LCDMenu_AddSubMenu(&menu, menu.root, "Settings", NULL);
 * LCDMenu_AddCallback(&menu, settings, "Reset", reset_fn);
 */
LCDMenu_Item* LCDMenu_AddSubMenu(LCDMenu_Handle* menu, LCDMenu_Item* parent,
                                  const char* name, void (*callback)(void));

/**
 * @brief เพิ่ม callback item (ทำงานเมื่อกด ENTER)
 * 
 * @param menu     ตัวชี้ไปยัง LCDMenu_Handle
 * @param parent   เมนูแม่
 * @param name     ชื่อที่แสดง
 * @param callback ฟังก์ชันที่จะเรียกเมื่อกด ENTER (NULL = ไม่มี action)
 * @return ตัวชี้ไปยัง item ที่สร้าง
 * 
 * @example
 * LCDMenu_AddCallback(&menu, menu.root, "Start", start_function);
 */
LCDMenu_Item* LCDMenu_AddCallback(LCDMenu_Handle* menu, LCDMenu_Item* parent,
                                   const char* name, void (*callback)(void));

/**
 * @brief เพิ่ม toggle item (ON/OFF)
 * 
 * @param menu     ตัวชี้ไปยัง LCDMenu_Handle
 * @param parent   เมนูแม่
 * @param name     ชื่อที่แสดง
 * @param value    ตัวชี้ไปยัง bool สำหรับเก็บค่า (ต้องเป็น static/global)
 * @param callback เรียกเมื่อค่าเปลี่ยน (NULL = ไม่ใช้)
 * @return ตัวชี้ไปยัง item ที่สร้าง
 * 
 * @note เมื่อกด ENTER จะสลับค่า true/false ทันที (ไม่เข้า edit mode)
 * 
 * @example
 * static bool backlight = true;
 * LCDMenu_AddToggle(&menu, settings, "Backlight", &backlight, on_bl_change);
 */
LCDMenu_Item* LCDMenu_AddToggle(LCDMenu_Handle* menu, LCDMenu_Item* parent,
                                 const char* name, bool* value,
                                 void (*callback)(void));

/**
 * @brief เพิ่ม value item (ปรับค่าตัวเลข)
 * 
 * @param menu     ตัวชี้ไปยัง LCDMenu_Handle
 * @param parent   เมนูแม่
 * @param name     ชื่อที่แสดง
 * @param value    ตัวชี้ไปยัง int32_t สำหรับเก็บค่า (ต้องเป็น static/global)
 * @param min      ค่าต่ำสุด
 * @param max      ค่าสูงสุด
 * @param step     ขั้นการปรับ (1, 5, 10, ฯลฯ)
 * @param callback เรียกเมื่อยืนยันค่า (NULL = ไม่ใช้)
 * @return ตัวชี้ไปยัง item ที่สร้าง
 * 
 * @note เมื่อกด ENTER จะเข้า edit mode — UP/DOWN ปรับค่า, ENTER ยืนยัน, BACK ยกเลิก
 * 
 * @example
 * static int32_t speed = 50;
 * LCDMenu_AddValue(&menu, settings, "Speed", &speed, 0, 100, 5, on_speed_change);
 */
LCDMenu_Item* LCDMenu_AddValue(LCDMenu_Handle* menu, LCDMenu_Item* parent,
                                const char* name, int32_t* value,
                                int32_t min, int32_t max, int32_t step,
                                void (*callback)(void));

/* === Runtime & Navigation === */

/**
 * @brief อัปเดตสถานะเมนู — ต้องเรียกใน main loop ทุก iteration
 * 
 * @param menu  ตัวชี้ไปยัง LCDMenu_Handle
 * 
 * @note ต้องเรียก Button_Update() สำหรับทุกปุ่มก่อนเรียกฟังก์ชันนี้
 * @note ฟังก์ชันนี้จะอ่าน event จากปุ่มด้วย Button_GetEvent() (polling mode)
 * 
 * @example
 * while (1) {
 *     Button_Update(&btn_up);
 *     Button_Update(&btn_down);
 *     Button_Update(&btn_enter);
 *     Button_Update(&btn_back);
 *     LCDMenu_Update(&menu);
 *     Delay_Ms(5);
 * }
 */
void LCDMenu_Update(LCDMenu_Handle* menu);

/**
 * @brief กลับไปยัง parent menu (หรือออกจาก edit mode)
 * 
 * @param menu  ตัวชี้ไปยัง LCDMenu_Handle
 * 
 * @note ถ้าอยู่ใน edit mode จะออกจาก edit mode ก่อน
 * @note ถ้าอยู่ที่ root แล้ว จะไม่มีผล
 */
void LCDMenu_Back(LCDMenu_Handle* menu);

/**
 * @brief กระโดดไปยังเมนูที่ระบุ
 * 
 * @param menu     ตัวชี้ไปยัง LCDMenu_Handle
 * @param target   เมนูปลายทาง (ต้องเป็น SUBMENU type)
 * 
 * @note ใช้สำหรับนำทางแบบโปรแกรม (ไม่ผ่านปุ่ม)
 * 
 * @example
 * LCDMenu_GoTo(&menu, settings_menu);
 */
void LCDMenu_GoTo(LCDMenu_Handle* menu, LCDMenu_Item* target);

/* === Display === */

/**
 * @brief วาดเมนูปัจจุบันใหม่ทั้งจอ
 * 
 * @param menu  ตัวชี้ไปยัง LCDMenu_Handle
 * 
 * @note ฟังก์ชันนี้จะถูกเรียกอัตโนมัติจาก LCDMenu_Update() เมื่อ state เปลี่ยน
 * @note สามารถเรียกเองได้เมื่อต้องการ force redraw (เช่น หลังจากเปลี่ยนค่าจากภายนอก)
 * 
 * @example
 * // เปลี่ยนค่า backlight จาก interrupt แล้ว force redraw
 * LCDMenu_Draw(&menu);
 */
void LCDMenu_Draw(LCDMenu_Handle* menu);

/**
 * @brief รีเซ็ต menu state (กลับไป root, cursor=0, ออกจาก edit mode)
 * 
 * @param menu  ตัวชี้ไปยัง LCDMenu_Handle
 */
void LCDMenu_Reset(LCDMenu_Handle* menu);

/**
 * @brief ตรวจสอบว่าอยู่ใน edit mode หรือไม่
 * 
 * @param menu  ตัวชี้ไปยัง LCDMenu_Handle
 * @return true ถ้าอยู่ใน edit mode
 */
bool LCDMenu_IsEditing(LCDMenu_Handle* menu);

/**
 * @brief คืนค่า item ปัจจุบันที่ cursor ชี้อยู่
 * 
 * @param menu  ตัวชี้ไปยัง LCDMenu_Handle
 * @return ตัวชี้ไปยัง item ปัจจุบัน (NULL ถ้าไม่มี)
 */
LCDMenu_Item* LCDMenu_GetCurrentItem(LCDMenu_Handle* menu);

#ifdef __cplusplus
}
#endif

#endif /* __LCDMENU_H */
