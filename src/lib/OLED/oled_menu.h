/**
 * @file oled_menu.h
 * @brief OLED Menu System - Interactive Menu Framework
 * @version 1.0
 * @date 2025-12-21
 * 
 * @details
 * Library สำหรับสร้างระบบเมนูแบบ interactive บน OLED
 * รองรับหลายประเภทเมนู และการนำทางด้วยปุ่ม
 * 
 * **คุณสมบัติ:**
 * - List menu
 * - Icon menu
 * - Settings menu
 * - Value editor (numeric, boolean, list)
 * - Callback system
 * - Scroll support
 * 
 * @example
 * #include "oled_menu.h"
 * 
 * void menu_callback1(void) { //action }
 * 
 * MenuItem items[] = {
 *     {"Option 1", menu_callback1, MENU_ITEM_ACTION},
 *     {"Option 2", NULL, MENU_ITEM_ACTION}
 * };
 * 
 * Menu menu;
 * OLED_MenuInit(&menu, items, 2);
 * OLED_MenuDraw(&oled, &menu);
 */

#ifndef __OLED_MENU_H
#define __OLED_MENU_H

#ifdef __cplusplus
extern "C" {
#endif

#include "oled_i2c.h"
#include "oled_fonts.h"
#include "oled_graphics.h"

/* ========== Menu Constants ========== */

#define MENU_MAX_ITEMS      16    /**< จำนวนรายการสูงสุด */
#define MENU_MAX_VISIBLE    4     /**< จำนวนรายการที่แสดงพร้อมกัน */
#define MENU_ITEM_HEIGHT    16    /**< ความสูงของแต่ละรายการ */

/* ========== Enumerations ========== */

/**
 * @brief ประเภทของรายการเมนู
 */
typedef enum {
    MENU_ITEM_ACTION = 0,   /**< รายการที่เรียก callback */
    MENU_ITEM_SUBMENU,      /**< รายการที่เปิดเมนูย่อย */
    MENU_ITEM_VALUE_INT,    /**< รายการแก้ไขค่า integer */
    MENU_ITEM_VALUE_BOOL,   /**< รายการแก้ไขค่า boolean */
    MENU_ITEM_VALUE_LIST,   /**< รายการเลือกจากลิสต์ */
    MENU_ITEM_BACK          /**< รายการกลับ */
} MenuItemType;

/**
 * @brief สไตล์การแสดงเมนู
 */
typedef enum {
    MENU_STYLE_LIST = 0,    /**< แบบลิสต์ */
    MENU_STYLE_ICON,        /**< แบบไอคอน */
    MENU_STYLE_FULL         /**< แบบเต็มจอ */
} MenuStyle;

/* ========== Structures ========== */

/**
 * @brief โครงสร้างสำหรับ Value Editor
 */
typedef struct {
    int32_t value;          /**< ค่าปัจจุบัน */
    int32_t min;            /**< ค่าต่ำสุด */
    int32_t max;            /**< ค่าสูงสุด */
    int32_t step;           /**< ขั้นการเปลี่ยนแปลง */
    const char** options;   /**< ตัวเลือก (สำหรับ VALUE_LIST) */
    uint8_t option_count;   /**< จำนวนตัวเลือก */
} MenuValue;

/**
 * @brief โครงสร้างสำหรับรายการเมนู
 */
typedef struct MenuItem {
    const char* text;           /**< ข้อความแสดง */
    void (*callback)(void);     /**< Callback function */
    MenuItemType type;          /**< ประเภทรายการ */
    const OLED_Bitmap* icon;    /**< ไอคอน (optional) */
    MenuValue* value;           /**< ตัวชี้ไปยัง value (สำหรับ VALUE types) */
    struct Menu* submenu;       /**< ตัวชี้ไปยังเมนูย่อย */
} MenuItem;

/**
 * @brief โครงสร้างสำหรับเมนู
 */
typedef struct Menu {
    MenuItem* items;            /**< Array ของรายการ */
    uint8_t item_count;         /**< จำนวนรายการ */
    uint8_t selected;           /**< รายการที่เลือก */
    uint8_t scroll_offset;      /**< ตำแหน่ง scroll */
    MenuStyle style;            /**< สไตล์การแสดง */
    uint8_t text_scale;         /**< ขนาดข้อความเมนู (1x-4x) */
    const char* title;          /**< หัวข้อเมนู */
    struct Menu* parent;        /**< ตัวชี้ไปยังเมนูหลัก */
} Menu;

/* ========== Menu Initialization ========== */

/**
 * @brief เริ่มต้นเมนู
 * @param menu ตัวชี้ไปยัง menu
 * @param items Array ของรายการเมนู
 * @param item_count จำนวนรายการ
 * 
 * @example
 * MenuItem items[] = {...};
 * Menu menu;
 * OLED_MenuInit(&menu, items, 3);
 */
void OLED_MenuInit(Menu* menu, MenuItem* items, uint8_t item_count);

/**
 * @brief ตั้งค่าหัวข้อเมนู
 * @param menu ตัวชี้ไปยัง menu
 * @param title หัวข้อ
 * 
 * @example
 * OLED_MenuSetTitle(&menu, "Settings");
 */
void OLED_MenuSetTitle(Menu* menu, const char* title);

/**
 * @brief ตั้งค่าสไตล์เมนู
 * @param menu ตัวชี้ไปยัง menu
 * @param style สไตล์
 * 
 * @example
 * OLED_MenuSetStyle(&menu, MENU_STYLE_ICON);
 */
void OLED_MenuSetStyle(Menu* menu, MenuStyle style);

/**
 * @brief ตั้งค่า text scale ของเมนู
 * @param menu ตัวชี้ไปยัง menu
 * @param scale ค่า 1..4
 */
void OLED_MenuSetTextScale(Menu* menu, uint8_t scale);

/* ========== Menu Navigation ========== */

/**
 * @brief เลื่อนไปรายการถัดไป
 * @param menu ตัวชี้ไปยัง menu
 * 
 * @example
 * OLED_MenuNext(&menu);
 */
void OLED_MenuNext(Menu* menu);

/**
 * @brief เลื่อนไปรายการก่อนหน้า
 * @param menu ตัวชี้ไปยัง menu
 * 
 * @example
 * OLED_MenuPrev(&menu);
 */
void OLED_MenuPrev(Menu* menu);

/**
 * @brief เลือกรายการปัจจุบัน
 * @param menu ตัวชี้ไปยัง menu
 * @return ตัวชี้ไปยังเมนูใหม่ (ถ้ามี submenu) หรือ NULL
 * 
 * @example
 * Menu* new_menu = OLED_MenuSelect(&menu);
 * if(new_menu) {
 *     current_menu = new_menu;
 * }
 */
Menu* OLED_MenuSelect(Menu* menu);

/**
 * @brief กลับไปเมนูหลัก
 * @param menu ตัวชี้ไปยัง menu
 * @return ตัวชี้ไปยังเมนูหลัก หรือ NULL
 * 
 * @example
 * Menu* parent = OLED_MenuBack(&menu);
 * if(parent) {
 *     current_menu = parent;
 * }
 */
Menu* OLED_MenuBack(Menu* menu);

/* ========== Value Editing ========== */

/**
 * @brief เพิ่มค่า
 * @param menu ตัวชี้ไปยัง menu
 * 
 * @note ใช้กับรายการประเภท VALUE
 * 
 * @example
 * OLED_MenuValueInc(&menu);
 */
void OLED_MenuValueInc(Menu* menu);

/**
 * @brief ลดค่า
 * @param menu ตัวชี้ไปยัง menu
 * 
 * @note ใช้กับรายการประเภท VALUE
 * 
 * @example
 * OLED_MenuValueDec(&menu);
 */
void OLED_MenuValueDec(Menu* menu);

/**
 * @brief สลับค่า boolean
 * @param menu ตัวชี้ไปยัง menu
 * 
 * @note ใช้กับรายการประเภท VALUE_BOOL
 * 
 * @example
 * OLED_MenuValueToggle(&menu);
 */
void OLED_MenuValueToggle(Menu* menu);

/* ========== Menu Drawing ========== */

/**
 * @brief วาดเมนู
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param menu ตัวชี้ไปยัง menu
 * 
 * @example
 * OLED_MenuDraw(&oled, &menu);
 */
void OLED_MenuDraw(OLED_Handle* oled, Menu* menu);

/**
 * @brief วาดเมนูแบบลิสต์
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param menu ตัวชี้ไปยัง menu
 */
void OLED_MenuDrawList(OLED_Handle* oled, Menu* menu);

/**
 * @brief วาดเมนูแบบไอคอน
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param menu ตัวชี้ไปยัง menu
 */
void OLED_MenuDrawIcon(OLED_Handle* oled, Menu* menu);

/**
 * @brief วาดเมนูแบบเต็มจอ
 * @param oled ตัวชี้ไปยัง OLED handle
 * @param menu ตัวชี้ไปยัง menu
 */
void OLED_MenuDrawFull(OLED_Handle* oled, Menu* menu);

/* ========== Helper Functions ========== */

/**
 * @brief สร้างรายการเมนู Action
 * @param text ข้อความ
 * @param callback Callback function
 * @return MenuItem
 * 
 * @example
 * MenuItem item = OLED_MenuCreateAction("Start", start_callback);
 */
MenuItem OLED_MenuCreateAction(const char* text, void (*callback)(void));

/**
 * @brief สร้างรายการเมนู Submenu
 * @param text ข้อความ
 * @param submenu ตัวชี้ไปยังเมนูย่อย
 * @return MenuItem
 * 
 * @example
 * MenuItem item = OLED_MenuCreateSubmenu("Settings", &settings_menu);
 */
MenuItem OLED_MenuCreateSubmenu(const char* text, Menu* submenu);

/**
 * @brief สร้างรายการเมนู Value (Integer)
 * @param text ข้อความ
 * @param value ตัวชี้ไปยัง MenuValue
 * @return MenuItem
 * 
 * @example
 * MenuValue val = {.value = 50, .min = 0, .max = 100, .step = 5};
 * MenuItem item = OLED_MenuCreateValueInt("Volume", &val);
 */
MenuItem OLED_MenuCreateValueInt(const char* text, MenuValue* value);

/**
 * @brief สร้างรายการเมนู Value (Boolean)
 * @param text ข้อความ
 * @param value ตัวชี้ไปยัง MenuValue
 * @return MenuItem
 * 
 * @example
 * MenuValue val = {.value = 1};
 * MenuItem item = OLED_MenuCreateValueBool("Enable", &val);
 */
MenuItem OLED_MenuCreateValueBool(const char* text, MenuValue* value);

#ifdef __cplusplus
}
#endif

#endif  // __OLED_MENU_H
