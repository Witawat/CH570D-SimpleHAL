/**
 * @file lcdmenu.c
 * @brief LCD Menu Library — Implementation
 * @version 1.0
 * @date 2026-05-04
 */

#include "lcdmenu.h"
#include <string.h>
#include <stdio.h>

/* ========== Cursor Custom Character (5x8) ========== */

/**
 * @brief ลูกศรขวา `>` สำหรับ cursor indicator
 * 
 * Bitmap 5x8 (5 LSBs ต่อ byte):
 * Row 0-7: ชี้ไปทางขวา
 */
static const uint8_t _cursor_char[8] = {
    0b00000,  // .....
    0b00100,  // ..#..
    0b00010,  // ...#.
    0b11111,  // #####
    0b00010,  // ...#.
    0b00100,  // ..#..
    0b00000,  // .....
    0b00000,  // .....
};

/* ========== Static Memory Pool ========== */

static LCDMenu_Item _menu_pool[LCDMENU_MAX_ITEMS];
static uint8_t _menu_pool_count = 0;

/**
 * @brief จอง item จาก static pool
 * @return ตัวชี้ไปยัง item ที่จองได้ (NULL ถ้าเต็ม)
 */
static LCDMenu_Item* _menu_alloc(void) {
    if (_menu_pool_count >= LCDMENU_MAX_ITEMS) {
        return NULL;
    }
    LCDMenu_Item* item = &_menu_pool[_menu_pool_count];
    memset(item, 0, sizeof(LCDMenu_Item));
    item->pool_index = _menu_pool_count;
    _menu_pool_count++;
    return item;
}

/* ========== Internal Helpers ========== */

/**
 * @brief ตรวจสอบว่า menu handle ถูก init แล้ว
 */
static bool _menu_is_initialized(LCDMenu_Handle* menu) {
    if (menu == NULL || !menu->initialized) return false;
    return true;
}

/**
 * @brief นับจำนวน items ในเมนู (นับ children ของ current_menu)
 */
static uint8_t _count_items(LCDMenu_Handle* menu) {
    if (menu->current_menu == NULL) return 0;
    return menu->current_menu->child_count;
}

/**
 * @brief คำนวณจำนวนแถวที่ใช้แสดง items (ไม่รวม title row)
 */
static uint8_t _rows_for_items(LCDMenu_Handle* menu) {
    if (menu->lcd->rows <= 1) return 1;
    return menu->lcd->rows - 1;  // เหลือ row 0 สำหรับ title
}

/**
 * @brief Clamp cursor_index ให้อยู่ในช่วงที่ valid
 */
static void _clamp_cursor(LCDMenu_Handle* menu) {
    uint8_t count = _count_items(menu);
    if (count == 0) {
        menu->cursor_index = -1;
        return;
    }
    if (menu->cursor_index < 0) menu->cursor_index = 0;
    if (menu->cursor_index >= (int8_t)count) menu->cursor_index = count - 1;
}

/**
 * @brief อัปเดต display_offset ให้ cursor อยู่ในหน้าจอ
 */
static void _update_offset(LCDMenu_Handle* menu) {
    uint8_t rows_for_items = _rows_for_items(menu);
    uint8_t count = _count_items(menu);
    
    if (count <= rows_for_items) {
        menu->display_offset = 0;
        return;
    }
    
    /* Ensure cursor is visible */
    if (menu->cursor_index < menu->display_offset) {
        menu->display_offset = menu->cursor_index;
    }
    if (menu->cursor_index >= menu->display_offset + (int8_t)rows_for_items) {
        menu->display_offset = menu->cursor_index - rows_for_items + 1;
    }
    
    /* Clamp offset */
    int8_t max_offset = count - rows_for_items;
    if (max_offset < 0) max_offset = 0;
    if (menu->display_offset < 0) menu->display_offset = 0;
    if (menu->display_offset > max_offset) menu->display_offset = max_offset;
}

/**
 * @brief ดึง child ที่ cursor_index ปัจจุบัน
 */
static LCDMenu_Item* _get_selected_item(LCDMenu_Handle* menu) {
    if (menu->current_menu == NULL) return NULL;
    if (menu->cursor_index < 0) return NULL;
    if (menu->cursor_index >= (int8_t)menu->current_menu->child_count) return NULL;
    return menu->current_menu->children[menu->cursor_index];
}

/**
 * @brief เขียนข้อความในบรรทัดที่กำหนด (clear line ก่อน)
 */
static void _write_line(LCDMenu_Handle* menu, uint8_t row, const char* text) {
    LCD_SetCursor(menu->lcd, 0, row);
    
    /* Write text padded with spaces to clear the line */
    uint8_t i;
    uint8_t cols = menu->lcd->cols;
    for (i = 0; i < cols; i++) {
        if (text[i] == '\0') {
            /* Fill remaining with spaces */
            while (i < cols) {
                LCD_PrintChar(menu->lcd, ' ');
                i++;
            }
            break;
        }
        LCD_PrintChar(menu->lcd, text[i]);
    }
}

/* ========== Render Title & Items ========== */

/**
 * @brief วาด title bar (บรรทัด 0)
 */
static void _draw_title(LCDMenu_Handle* menu) {
    char buf[21];  /* max 20 cols + null */
    uint8_t cols = menu->lcd->cols;
    
    if (menu->current_menu == NULL || menu->current_menu->name == NULL) {
        /* No title */
        memset(buf, ' ', cols);
        buf[cols] = '\0';
    } else {
        /* Center the title */
        const char* name = menu->current_menu->name;
        uint8_t name_len = (uint8_t)strlen(name);
        if (name_len > cols) name_len = cols;
        
        int8_t pad_left = (cols - name_len) / 2;
        if (pad_left < 0) pad_left = 0;
        
        memset(buf, ' ', cols);
        buf[cols] = '\0';
        memcpy(buf + pad_left, name, name_len);
    }
    
    _write_line(menu, 0, buf);
}

/**
 * @brief ดึงข้อความสำหรับ 1 menu item (รวม prefix/suffix)
 */
static void _format_item_text(LCDMenu_Handle* menu, LCDMenu_Item* item,
                               bool is_selected, char* buf, uint8_t buf_size) {
    uint8_t cols = menu->lcd->cols;
    uint8_t pos = 0;
    
    /* Prefix: cursor indicator or space */
    if (is_selected) {
        buf[pos++] = (char)menu->cgram_cursor;  /* custom char */
    } else {
        buf[pos++] = ' ';
    }
    buf[pos++] = ' ';  /* spacing */
    
    /* Item name (truncate if needed) */
    const char* name = item->name ? item->name : "???";
    uint8_t name_len = (uint8_t)strlen(name);
    
    /* Reserve space for suffix */
    uint8_t suffix_len = 0;
    char suffix[8] = "";
    
    if (is_selected && menu->edit_mode) {
        if (item->type == LCDMENU_TYPE_VALUE) {
            snprintf(suffix, sizeof(suffix), "<%ld>", (long)menu->edit_value);
        }
    } else {
        switch (item->type) {
            case LCDMENU_TYPE_TOGGLE:
                if (item->bool_value && *(item->bool_value)) {
                    strcpy(suffix, "[ON]");
                } else {
                    strcpy(suffix, "[OFF]");
                }
                break;
            case LCDMENU_TYPE_VALUE:
                if (item->int32_value) {
                    snprintf(suffix, sizeof(suffix), "%ld", (long)*(item->int32_value));
                }
                break;
            case LCDMENU_TYPE_SUBMENU:
                suffix[0] = '>';
                suffix[1] = '\0';
                break;
            default:
                break;
        }
    }
    suffix_len = (uint8_t)strlen(suffix);
    
    /* Calculate available space for name */
    uint8_t avail = cols - pos - suffix_len;
    if (avail > cols) avail = 0;  /* overflow guard */
    
    if (name_len > avail) {
        name_len = avail;
    }
    
    memcpy(buf + pos, name, name_len);
    pos += name_len;
    
    /* Right-align suffix */
    uint8_t suffix_pos = cols - suffix_len;
    while (pos < suffix_pos && pos < cols) {
        buf[pos++] = ' ';
    }
    memcpy(buf + pos, suffix, suffix_len);
    pos += suffix_len;
    
    buf[pos] = '\0';
}

/**
 * @brief วาดรายการเมนู (rows 1+)
 */
static void _draw_items(LCDMenu_Handle* menu) {
    uint8_t rows_for_items = _rows_for_items(menu);
    uint8_t count = _count_items(menu);
    uint8_t cols = menu->lcd->cols;
    char buf[21];  /* max 20 cols + null */
    
    for (uint8_t row = 0; row < rows_for_items; row++) {
        int16_t item_idx = menu->display_offset + row;
        
        if (item_idx >= 0 && item_idx < (int16_t)count) {
            LCDMenu_Item* item = menu->current_menu->children[item_idx];
            bool is_selected = (item_idx == menu->cursor_index);
            
            _format_item_text(menu, item, is_selected, buf, sizeof(buf));
            _write_line(menu, row + 1, buf);
        } else {
            /* Empty line */
            memset(buf, ' ', cols);
            buf[cols] = '\0';
            _write_line(menu, row + 1, buf);
        }
    }
}

/* ========== Public API ========== */

/**
 * @brief เริ่มต้น LCDMenu
 */
void LCDMenu_Init(LCDMenu_Handle* menu,
                  LCD1602_Handle* lcd,
                  Button_Instance* btn_up,
                  Button_Instance* btn_down,
                  Button_Instance* btn_enter,
                  Button_Instance* btn_back) {
    if (menu == NULL || lcd == NULL) return;
    
    memset(menu, 0, sizeof(LCDMenu_Handle));
    
    menu->lcd       = lcd;
    menu->btn_up    = btn_up;
    menu->btn_down  = btn_down;
    menu->btn_enter = btn_enter;
    menu->btn_back  = btn_back;
    
    /* Determine visible rows */
    menu->visible_rows = lcd->rows;
    menu->title_row = 0;
    
    /* Default state */
    menu->cursor_index  = 0;
    menu->display_offset = 0;
    menu->edit_mode     = false;
    menu->dirty         = 1;
    menu->initialized   = 1;
    
    /* Create cursor custom character at CGRAM location 0 */
    menu->cgram_cursor = 0;
    LCD_CreateChar(menu->lcd, menu->cgram_cursor, (uint8_t*)_cursor_char);
}

/**
 * @brief สร้าง root menu
 */
void LCDMenu_CreateRoot(LCDMenu_Handle* menu, const char* title) {
    if (!_menu_is_initialized(menu)) return;
    
    LCDMenu_Item* root = _menu_alloc();
    if (root == NULL) return;
    
    root->name        = title;
    root->type        = LCDMENU_TYPE_SUBMENU;
    root->parent      = NULL;
    root->child_count = 0;
    root->initialized = 1;
    
    menu->root          = root;
    menu->current_menu  = root;
    menu->cursor_index  = 0;
    menu->display_offset = 0;
    menu->edit_mode     = false;
    menu->dirty         = 1;
}

/**
 * @brief เพิ่ม item เข้า children ของ parent (internal)
 */
static LCDMenu_Item* _add_item(LCDMenu_Handle* menu, LCDMenu_Item* parent,
                                const char* name, LCDMenu_ItemType type) {
    if (!_menu_is_initialized(menu)) return NULL;
    if (parent == NULL) return NULL;
    if (parent->child_count >= LCDMENU_MAX_CHILDREN) return NULL;
    
    LCDMenu_Item* item = _menu_alloc();
    if (item == NULL) return NULL;
    
    item->name        = name;
    item->type        = type;
    item->parent      = parent;
    item->child_count = 0;
    item->initialized = 1;
    
    parent->children[parent->child_count] = item;
    parent->child_count++;
    
    return item;
}

/**
 * @brief เพิ่ม sub-menu item
 */
LCDMenu_Item* LCDMenu_AddSubMenu(LCDMenu_Handle* menu, LCDMenu_Item* parent,
                                  const char* name, void (*callback)(void)) {
    LCDMenu_Item* item = _add_item(menu, parent, name, LCDMENU_TYPE_SUBMENU);
    if (item) {
        item->callback = callback;
    }
    return item;
}

/**
 * @brief เพิ่ม callback item
 */
LCDMenu_Item* LCDMenu_AddCallback(LCDMenu_Handle* menu, LCDMenu_Item* parent,
                                   const char* name, void (*callback)(void)) {
    LCDMenu_Item* item = _add_item(menu, parent, name, LCDMENU_TYPE_CALLBACK);
    if (item) {
        item->callback = callback;
    }
    return item;
}

/**
 * @brief เพิ่ม toggle item
 */
LCDMenu_Item* LCDMenu_AddToggle(LCDMenu_Handle* menu, LCDMenu_Item* parent,
                                 const char* name, bool* value,
                                 void (*callback)(void)) {
    LCDMenu_Item* item = _add_item(menu, parent, name, LCDMENU_TYPE_TOGGLE);
    if (item) {
        item->bool_value = value;
        item->callback   = callback;
    }
    return item;
}

/**
 * @brief เพิ่ม value item
 */
LCDMenu_Item* LCDMenu_AddValue(LCDMenu_Handle* menu, LCDMenu_Item* parent,
                                const char* name, int32_t* value,
                                int32_t min, int32_t max, int32_t step,
                                void (*callback)(void)) {
    LCDMenu_Item* item = _add_item(menu, parent, name, LCDMENU_TYPE_VALUE);
    if (item) {
        item->int32_value = value;
        item->value_min   = min;
        item->value_max   = max;
        item->value_step  = (step > 0) ? step : 1;
        item->callback    = callback;
    }
    return item;
}

/* ========== Navigation Logic ========== */

/**
 * @brief Handle UP button
 */
static uint8_t _handle_up(LCDMenu_Handle* menu) {
    uint8_t count = _count_items(menu);
    if (count == 0) return 0;
    
    if (menu->cursor_index > 0) {
        menu->cursor_index--;
        _update_offset(menu);
        return 1;
    }
    /* Wrap around to bottom */
    menu->cursor_index = count - 1;
    _update_offset(menu);
    return 1;
}

/**
 * @brief Handle DOWN button
 */
static uint8_t _handle_down(LCDMenu_Handle* menu) {
    uint8_t count = _count_items(menu);
    if (count == 0) return 0;
    
    if (menu->cursor_index < (int8_t)(count - 1)) {
        menu->cursor_index++;
        _update_offset(menu);
        return 1;
    }
    /* Wrap around to top */
    menu->cursor_index = 0;
    _update_offset(menu);
    return 1;
}

/**
 * @brief Handle ENTER button (normal mode)
 */
static uint8_t _handle_enter(LCDMenu_Handle* menu) {
    LCDMenu_Item* item = _get_selected_item(menu);
    if (item == NULL) return 0;
    
    switch (item->type) {
        case LCDMENU_TYPE_SUBMENU:
            /* เข้า sub-menu */
            if (item->child_count > 0) {
                menu->current_menu   = item;
                menu->cursor_index   = 0;
                menu->display_offset = 0;
                if (item->callback) item->callback();
                return 1;
            }
            /* Sub-menu ว่าง — fall through เป็น callback */
            if (item->callback) {
                item->callback();
                return 1;
            }
            return 0;
            
        case LCDMENU_TYPE_CALLBACK:
            /* เรียก callback */
            if (item->callback) {
                item->callback();
            }
            return 1;
            
        case LCDMENU_TYPE_TOGGLE:
            /* สลับค่า ON/OFF ทันที */
            if (item->bool_value) {
                *(item->bool_value) = !(*(item->bool_value));
                if (item->callback) item->callback();
            }
            return 1;
            
        case LCDMENU_TYPE_VALUE:
            /* เข้า edit mode */
            if (item->int32_value) {
                menu->edit_mode  = true;
                menu->edit_value = *(item->int32_value);
            }
            return 1;
            
        default:
            return 0;
    }
}

/**
 * @brief Handle ENTER button (edit mode — confirm)
 */
static uint8_t _handle_enter_edit(LCDMenu_Handle* menu) {
    LCDMenu_Item* item = _get_selected_item(menu);
    if (item == NULL || item->type != LCDMENU_TYPE_VALUE) {
        menu->edit_mode = false;
        return 1;
    }
    
    if (item->int32_value) {
        *(item->int32_value) = menu->edit_value;
        if (item->callback) item->callback();
    }
    menu->edit_mode = false;
    return 1;
}

/**
 * @brief Handle BACK button (normal mode)
 */
static uint8_t _handle_back(LCDMenu_Handle* menu) {
    if (menu->current_menu != NULL && menu->current_menu->parent != NULL) {
        /* กลับไป parent menu */
        menu->current_menu   = menu->current_menu->parent;
        menu->cursor_index   = 0;
        menu->display_offset = 0;
        return 1;
    }
    return 0;
}

/**
 * @brief Handle BACK button (edit mode — cancel)
 */
static uint8_t _handle_back_edit(LCDMenu_Handle* menu) {
    menu->edit_mode  = false;
    menu->edit_value = 0;
    return 1;
}

/* ========== Main Update ========== */

/**
 * @brief อัปเดตสถานะเมนู
 */
void LCDMenu_Update(LCDMenu_Handle* menu) {
    if (!_menu_is_initialized(menu)) return;
    
    Button_Event ev;
    uint8_t changed = 0;
    
    /* ===== Edit Mode Input ===== */
    if (menu->edit_mode) {
        LCDMenu_Item* item = _get_selected_item(menu);
        
        /* UP = increment value */
        if (menu->btn_up) {
            ev = Button_GetEvent(menu->btn_up);
            if (ev == BUTTON_EVENT_PRESS || ev == BUTTON_EVENT_LONG_PRESS) {
                if (item && item->type == LCDMENU_TYPE_VALUE) {
                    menu->edit_value += item->value_step;
                    if (menu->edit_value > item->value_max) {
                        menu->edit_value = item->value_max;
                    }
                    changed = 1;
                }
            }
        }
        
        /* DOWN = decrement value */
        if (menu->btn_down) {
            ev = Button_GetEvent(menu->btn_down);
            if (ev == BUTTON_EVENT_PRESS || ev == BUTTON_EVENT_LONG_PRESS) {
                if (item && item->type == LCDMENU_TYPE_VALUE) {
                    menu->edit_value -= item->value_step;
                    if (menu->edit_value < item->value_min) {
                        menu->edit_value = item->value_min;
                    }
                    changed = 1;
                }
            }
        }
        
        /* ENTER = confirm */
        if (menu->btn_enter) {
            ev = Button_GetEvent(menu->btn_enter);
            if (ev == BUTTON_EVENT_PRESS) {
                changed = _handle_enter_edit(menu);
            }
        }
        
        /* BACK = cancel */
        if (menu->btn_back) {
            ev = Button_GetEvent(menu->btn_back);
            if (ev == BUTTON_EVENT_PRESS) {
                changed = _handle_back_edit(menu);
            }
        }
    }
    /* ===== Normal Mode Input ===== */
    else {
        /* UP */
        if (menu->btn_up) {
            ev = Button_GetEvent(menu->btn_up);
            if (ev == BUTTON_EVENT_PRESS || ev == BUTTON_EVENT_LONG_PRESS) {
                changed = _handle_up(menu);
            }
        }
        
        /* DOWN */
        if (menu->btn_down) {
            ev = Button_GetEvent(menu->btn_down);
            if (ev == BUTTON_EVENT_PRESS || ev == BUTTON_EVENT_LONG_PRESS) {
                changed = _handle_down(menu);
            }
        }
        
        /* ENTER */
        if (menu->btn_enter) {
            ev = Button_GetEvent(menu->btn_enter);
            if (ev == BUTTON_EVENT_PRESS) {
                changed = _handle_enter(menu);
            }
        }
        
        /* BACK */
        if (menu->btn_back) {
            ev = Button_GetEvent(menu->btn_back);
            if (ev == BUTTON_EVENT_PRESS) {
                changed = _handle_back(menu);
            }
        }
    }
    
    /* Redraw if state changed */
    if (changed) {
        menu->dirty = 1;
    }
    
    if (menu->dirty) {
        LCDMenu_Draw(menu);
        menu->dirty = 0;
    }
}

/* ========== Navigation Functions ========== */

/**
 * @brief กลับไปยัง parent menu
 */
void LCDMenu_Back(LCDMenu_Handle* menu) {
    if (!_menu_is_initialized(menu)) return;
    
    if (menu->edit_mode) {
        _handle_back_edit(menu);
    } else {
        _handle_back(menu);
    }
    menu->dirty = 1;
    LCDMenu_Draw(menu);
    menu->dirty = 0;
}

/**
 * @brief กระโดดไปยังเมนูที่ระบุ
 */
void LCDMenu_GoTo(LCDMenu_Handle* menu, LCDMenu_Item* target) {
    if (!_menu_is_initialized(menu)) return;
    if (target == NULL) return;
    if (target->type != LCDMENU_TYPE_SUBMENU) return;
    
    menu->current_menu   = target;
    menu->cursor_index   = 0;
    menu->display_offset = 0;
    menu->edit_mode      = false;
    menu->dirty          = 1;
    LCDMenu_Draw(menu);
    menu->dirty = 0;
}

/* ========== Display ========== */

/**
 * @brief วาดเมนูปัจจุบันใหม่ทั้งจอ
 */
void LCDMenu_Draw(LCDMenu_Handle* menu) {
    if (!_menu_is_initialized(menu)) return;
    
    _clamp_cursor(menu);
    _update_offset(menu);
    
    _draw_title(menu);
    _draw_items(menu);
}

/* ========== Utility Functions ========== */

/**
 * @brief รีเซ็ต menu state
 */
void LCDMenu_Reset(LCDMenu_Handle* menu) {
    if (!_menu_is_initialized(menu)) return;
    
    menu->current_menu   = menu->root;
    menu->cursor_index   = 0;
    menu->display_offset = 0;
    menu->edit_mode      = false;
    menu->dirty          = 1;
    LCDMenu_Draw(menu);
    menu->dirty = 0;
}

/**
 * @brief ตรวจสอบ edit mode
 */
bool LCDMenu_IsEditing(LCDMenu_Handle* menu) {
    if (!_menu_is_initialized(menu)) return false;
    return menu->edit_mode;
}

/**
 * @brief คืนค่า item ปัจจุบัน
 */
LCDMenu_Item* LCDMenu_GetCurrentItem(LCDMenu_Handle* menu) {
    if (!_menu_is_initialized(menu)) return NULL;
    return _get_selected_item(menu);
}
