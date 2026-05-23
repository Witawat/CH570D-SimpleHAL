/**
 * @file oled_menu.c
 * @brief OLED Menu System Implementation
 * @version 1.0
 * @date 2025-12-21
 */

#include "oled_menu.h"

static uint8_t _menu_clamp_scale(uint8_t scale) {
    if(scale < 1) return 1;
    if(scale > 4) return 4;
    return scale;
}

static OLED_TextConfig _menu_text_cfg(const Menu* menu) {
    OLED_TextConfig cfg;
    cfg.scale = _menu_clamp_scale(menu->text_scale);
    cfg.letter_spacing = 0;
    cfg.thai_mode = OLED_THAI_RENDER_COMBINING;
    return cfg;
}

/* ========== Menu Initialization ========== */

/**
 * @brief Initialises menu system with menu items
 *
 * @param menu - pointer to Menu struct
 * @param items - array of MenuItem
 * @param item_count - number of items
 *
 * @note ตั้งค่า default: style = LIST, text_scale = 1, title = NULL
 *       เชื่อม parent reference สำหรับ submenus
 */
void OLED_MenuInit(Menu* menu, MenuItem* items, uint8_t item_count) {
    menu->items = items;
    menu->item_count = item_count;
    menu->selected = 0;
    menu->scroll_offset = 0;
    menu->style = MENU_STYLE_LIST;
    menu->text_scale = 1;
    menu->title = NULL;
    menu->parent = NULL;
    
    // Set parent reference for submenus
    for(uint8_t i = 0; i < item_count; i++) {
        if(items[i].type == MENU_ITEM_SUBMENU && items[i].submenu != NULL) {
            items[i].submenu->parent = menu;
        }
    }
}

/**
 * @brief Sets menu title
 *
 * @param menu - pointer to Menu
 * @param title - title string
 */
void OLED_MenuSetTitle(Menu* menu, const char* title) {
    menu->title = title;
}

/**
 * @brief Sets menu display style
 *
 * @param menu - pointer to Menu
 * @param style - MENU_STYLE_LIST / ICON / FULL
 */
void OLED_MenuSetStyle(Menu* menu, MenuStyle style) {
    menu->style = style;
}

/**
 * @brief Sets menu text scale
 *
 * @param menu - pointer to Menu
 * @param scale - scale 1–4 (clamped)
 */
void OLED_MenuSetTextScale(Menu* menu, uint8_t scale) {
    menu->text_scale = _menu_clamp_scale(scale);
}

/* ========== Menu Navigation ========== */

/**
 * @brief Moves selection to next item (with auto-scroll)
 *
 * @param menu - pointer to Menu
 *
 * @note auto-scroll เมื่อ selection เกิน MENU_MAX_VISIBLE
 */
void OLED_MenuNext(Menu* menu) {
    if(menu->selected < menu->item_count - 1) {
        menu->selected++;
        
        // Auto-scroll
        if(menu->selected >= menu->scroll_offset + MENU_MAX_VISIBLE) {
            menu->scroll_offset = menu->selected - MENU_MAX_VISIBLE + 1;
        }
    }
}

/**
 * @brief Moves selection to previous item (with auto-scroll)
 *
 * @param menu - pointer to Menu
 *
 * @note auto-scroll เมื่อ selection ต่ำกว่า scroll_offset
 */
void OLED_MenuPrev(Menu* menu) {
    if(menu->selected > 0) {
        menu->selected--;
        
        // Auto-scroll
        if(menu->selected < menu->scroll_offset) {
            menu->scroll_offset = menu->selected;
        }
    }
}

/**
 * @brief Selects current menu item (executes action or opens submenu)
 *
 * @param menu - pointer to Menu
 *
 * @return submenu if MENU_ITEM_SUBMENU, parent if MENU_ITEM_BACK, NULL otherwise
 *
 * @note ACTION → execute callback
 *       SUBMENU → return submenu pointer
 *       BACK → return parent pointer
 */
Menu* OLED_MenuSelect(Menu* menu) {
    MenuItem* item = &menu->items[menu->selected];
    
    switch(item->type) {
        case MENU_ITEM_ACTION:
            if(item->callback != NULL) {
                item->callback();
            }
            return NULL;
            
        case MENU_ITEM_SUBMENU:
            return item->submenu;
            
        case MENU_ITEM_BACK:
            return menu->parent;
            
        default:
            return NULL;
    }
}

/**
 * @brief Returns to parent menu
 *
 * @param menu - pointer to Menu
 *
 * @return parent Menu pointer (NULL if no parent)
 */
Menu* OLED_MenuBack(Menu* menu) {
    return menu->parent;
}

/* ========== Value Editing ========== */

/**
 * @brief Increments value of currently selected value item
 *
 * @param menu - pointer to Menu
 *
 * @note INT → value += step (clamped to max)
 *       LIST → value++ (clamped to option_count-1)
 *       อื่น ๆ → no-op
 */
void OLED_MenuValueInc(Menu* menu) {
    MenuItem* item = &menu->items[menu->selected];
    
    if(item->value == NULL) return;
    
    switch(item->type) {
        case MENU_ITEM_VALUE_INT:
            if(item->value->value < item->value->max) {
                item->value->value += item->value->step;
                if(item->value->value > item->value->max) {
                    item->value->value = item->value->max;
                }
            }
            break;
            
        case MENU_ITEM_VALUE_LIST:
            if(item->value->value < item->value->option_count - 1) {
                item->value->value++;
            }
            break;
            
        default:
            break;
    }
}

/**
 * @brief Decrements value of currently selected value item
 *
 * @param menu - pointer to Menu
 *
 * @note INT → value -= step (clamped to min)
 *       LIST → value-- (clamped to 0)
 *       อื่น ๆ → no-op
 */
void OLED_MenuValueDec(Menu* menu) {
    MenuItem* item = &menu->items[menu->selected];
    
    if(item->value == NULL) return;
    
    switch(item->type) {
        case MENU_ITEM_VALUE_INT:
            if(item->value->value > item->value->min) {
                item->value->value -= item->value->step;
                if(item->value->value < item->value->min) {
                    item->value->value = item->value->min;
                }
            }
            break;
            
        case MENU_ITEM_VALUE_LIST:
            if(item->value->value > 0) {
                item->value->value--;
            }
            break;
            
        default:
            break;
    }
}

/**
 * @brief Toggles boolean value item
 *
 * @param menu - pointer to Menu
 *
 * @note ใช้ได้เฉพาะ MENU_ITEM_VALUE_BOOL
 */
void OLED_MenuValueToggle(Menu* menu) {
    MenuItem* item = &menu->items[menu->selected];
    
    if(item->type == MENU_ITEM_VALUE_BOOL && item->value != NULL) {
        item->value->value = !item->value->value;
    }
}

/* ========== Menu Drawing ========== */

/**
 * @brief Draws menu on OLED (dispatches to style-specific drawer)
 *
 * @param oled - pointer to OLED_Handle
 * @param menu - pointer to Menu
 *
 * @note เรียก drawer ตาม style: LIST / ICON / FULL
 */
void OLED_MenuDraw(OLED_Handle* oled, Menu* menu) {
    switch(menu->style) {
        case MENU_STYLE_ICON:
            OLED_MenuDrawIcon(oled, menu);
            break;
        case MENU_STYLE_FULL:
            OLED_MenuDrawFull(oled, menu);
            break;
        case MENU_STYLE_LIST:
        default:
            OLED_MenuDrawList(oled, menu);
            break;
    }
}

/**
 * @brief Draws list-style menu (item list with selection highlight)
 *
 * @param oled - pointer to OLED_Handle
 * @param menu - pointer to Menu
 *
 * @note แสดง title ถ้ามี, selection = highlighted bar,
 *       value items แสดงค่าทางขวา, submenu แสดง '>'
 *       scroll indicators: ^ / v เมื่อมี items นอกจอ
 */
void OLED_MenuDrawList(OLED_Handle* oled, Menu* menu) {
    uint8_t y = 0;
    OLED_TextConfig text_cfg = _menu_text_cfg(menu);
    uint8_t row_h = (uint8_t)(MENU_ITEM_HEIGHT * text_cfg.scale);
    
    // Draw title if present
    if(menu->title != NULL) {
        uint16_t title_w = OLED_MeasureStringUTF8(oled, menu->title, &text_cfg);
        uint8_t title_x = (oled->width > title_w) ? (uint8_t)((oled->width - title_w) / 2) : 0;
        OLED_DrawStringUTF8Ex(oled, title_x, 0, menu->title, OLED_COLOR_WHITE, &text_cfg);
        OLED_DrawHLine(oled, 0, 10, oled->width, OLED_COLOR_WHITE);
        y = 12;
    }
    
    // Draw menu items
    uint8_t visible_count = (menu->title != NULL) ? 3 : MENU_MAX_VISIBLE;
    
    for(uint8_t i = 0; i < visible_count && (i + menu->scroll_offset) < menu->item_count; i++) {
        uint8_t item_index = i + menu->scroll_offset;
        MenuItem* item = &menu->items[item_index];
        uint8_t item_y = y + i * row_h;
        
        // Draw selection indicator
        if(item_index == menu->selected) {
            OLED_FillRect(oled, 0, item_y, oled->width, row_h, OLED_COLOR_WHITE);
            OLED_DrawStringUTF8Ex(oled, 4, (uint8_t)(item_y + 2), item->text, OLED_COLOR_BLACK, &text_cfg);
        } else {
            OLED_DrawStringUTF8Ex(oled, 4, (uint8_t)(item_y + 2), item->text, OLED_COLOR_WHITE, &text_cfg);
        }
        
        // Draw value if applicable
        if(item->value != NULL) {
            char value_str[16];
            
            switch(item->type) {
                case MENU_ITEM_VALUE_INT:
                    snprintf(value_str, sizeof(value_str), "%ld", (long)item->value->value);
                    break;
                    
                case MENU_ITEM_VALUE_BOOL:
                    snprintf(value_str, sizeof(value_str), "%s", item->value->value ? "ON" : "OFF");
                    break;
                    
                case MENU_ITEM_VALUE_LIST:
                    if(item->value->options != NULL && item->value->value < item->value->option_count) {
                        snprintf(value_str, sizeof(value_str), "%s", item->value->options[item->value->value]);
                    }
                    break;
                    
                default:
                    value_str[0] = '\0';
                    break;
            }
            
            if(value_str[0] != '\0') {
                OLED_Color color = (item_index == menu->selected) ? OLED_COLOR_BLACK : OLED_COLOR_WHITE;
                OLED_DrawStringAlign(oled, oled->width - 4, item_y + 2, value_str, color, OLED_ALIGN_RIGHT);
            }
        }
        
        // Draw submenu indicator
        if(item->type == MENU_ITEM_SUBMENU) {
            OLED_Color color = (item_index == menu->selected) ? OLED_COLOR_BLACK : OLED_COLOR_WHITE;
            OLED_DrawString(oled, oled->width - 10, item_y + 2, ">", color);
        }
    }
    
    // Draw scroll indicators
    if(menu->scroll_offset > 0) {
        OLED_DrawString(oled, oled->width / 2, y, "^", OLED_COLOR_WHITE);
    }
    if(menu->scroll_offset + visible_count < menu->item_count) {
        OLED_DrawString(oled, oled->width / 2, oled->height - 8, "v", OLED_COLOR_WHITE);
    }
}

/**
 * @brief Draws icon-style menu (icon + text, left/right navigation)
 *
 * @param oled - pointer to OLED_Handle
 * @param menu - pointer to Menu
 *
 * @note แสดงไอคอน (bitmap) หรือกรอบสี่เหลี่ยมถ้าไม่มี
 *       แสดงลูกศร < > สำหรับ navigation
 */
void OLED_MenuDrawIcon(OLED_Handle* oled, Menu* menu) {
    // Icon menu - simplified implementation
    uint8_t icon_size = 32;
    uint8_t spacing = 8;
    uint8_t start_x = (oled->width - icon_size) / 2;
    uint8_t start_y = 16;
    
    MenuItem* item = &menu->items[menu->selected];
    OLED_TextConfig text_cfg = _menu_text_cfg(menu);
    
    // Draw icon if present
    if(item->icon != NULL) {
        OLED_DrawBitmap(oled, start_x, start_y, item->icon, OLED_COLOR_WHITE);
    } else {
        OLED_DrawRect(oled, start_x, start_y, icon_size, icon_size, OLED_COLOR_WHITE);
    }
    
    // Draw text
    {
        uint16_t text_w = OLED_MeasureStringUTF8(oled, item->text, &text_cfg);
        uint8_t text_x = (oled->width > text_w) ? (uint8_t)((oled->width - text_w) / 2) : 0;
        OLED_DrawStringUTF8Ex(oled, text_x, (uint8_t)(start_y + icon_size + spacing), item->text, OLED_COLOR_WHITE, &text_cfg);
    }
    
    // Draw navigation indicators
    if(menu->selected > 0) {
        OLED_DrawString(oled, 0, oled->height / 2, "<", OLED_COLOR_WHITE);
    }
    if(menu->selected < menu->item_count - 1) {
        OLED_DrawString(oled, oled->width - 8, oled->height / 2, ">", OLED_COLOR_WHITE);
    }
}

/**
 * @brief Draws full-screen menu (title + large value display)
 *
 * @param oled - pointer to OLED_Handle
 * @param menu - pointer to Menu
 *
 * @note แสดงชื่อ item เป็น title, ค่า value ตรงกลางจอ
 *       มีเส้นแบ่งใต้ title
 */
void OLED_MenuDrawFull(OLED_Handle* oled, Menu* menu) {
    MenuItem* item = &menu->items[menu->selected];
    OLED_TextConfig text_cfg = _menu_text_cfg(menu);
    
    // Draw title
    {
        uint16_t title_w = OLED_MeasureStringUTF8(oled, item->text, &text_cfg);
        uint8_t title_x = (oled->width > title_w) ? (uint8_t)((oled->width - title_w) / 2) : 0;
        OLED_DrawStringUTF8Ex(oled, title_x, 0, item->text, OLED_COLOR_WHITE, &text_cfg);
    }
    OLED_DrawHLine(oled, 0, 10, oled->width, OLED_COLOR_WHITE);
    
    // Draw value/content in center
    if(item->value != NULL) {
        char value_str[32];
        
        switch(item->type) {
            case MENU_ITEM_VALUE_INT:
                snprintf(value_str, sizeof(value_str), "%ld", (long)item->value->value);
                break;
                
            case MENU_ITEM_VALUE_BOOL:
                snprintf(value_str, sizeof(value_str), "%s", item->value->value ? "ON" : "OFF");
                break;
                
            case MENU_ITEM_VALUE_LIST:
                if(item->value->options != NULL && item->value->value < item->value->option_count) {
                    snprintf(value_str, sizeof(value_str), "%s", item->value->options[item->value->value]);
                }
                break;
                
            default:
                value_str[0] = '\0';
                break;
        }
        
        OLED_DrawStringAlign(oled, oled->width / 2, oled->height / 2 - 4, 
                             value_str, OLED_COLOR_WHITE, OLED_ALIGN_CENTER);
    }
}

/* ========== Helper Functions ========== */

/**
 * @brief Creates an action menu item (executes callback on select)
 *
 * @param text - display text
 * @param callback - function pointer to execute
 *
 * @return MenuItem with type = MENU_ITEM_ACTION
 */
MenuItem OLED_MenuCreateAction(const char* text, void (*callback)(void)) {
    MenuItem item = {
        .text = text,
        .callback = callback,
        .type = MENU_ITEM_ACTION,
        .icon = NULL,
        .value = NULL,
        .submenu = NULL
    };
    return item;
}

/**
 * @brief Creates a submenu item (navigates to submenu on select)
 *
 * @param text - display text
 * @param submenu - pointer to child Menu
 *
 * @return MenuItem with type = MENU_ITEM_SUBMENU
 */
MenuItem OLED_MenuCreateSubmenu(const char* text, Menu* submenu) {
    MenuItem item = {
        .text = text,
        .callback = NULL,
        .type = MENU_ITEM_SUBMENU,
        .icon = NULL,
        .value = NULL,
        .submenu = submenu
    };
    return item;
}

/**
 * @brief Creates an integer value item (editable via Inc/Dec)
 *
 * @param text - display text
 * @param value - pointer to MenuValue (contains min/max/step/value)
 *
 * @return MenuItem with type = MENU_ITEM_VALUE_INT
 */
MenuItem OLED_MenuCreateValueInt(const char* text, MenuValue* value) {
    MenuItem item = {
        .text = text,
        .callback = NULL,
        .type = MENU_ITEM_VALUE_INT,
        .icon = NULL,
        .value = value,
        .submenu = NULL
    };
    return item;
}

/**
 * @brief Creates a boolean value item (toggle on/off)
 *
 * @param text - display text
 * @param value - pointer to MenuValue
 *
 * @return MenuItem with type = MENU_ITEM_VALUE_BOOL
 */
MenuItem OLED_MenuCreateValueBool(const char* text, MenuValue* value) {
    MenuItem item = {
        .text = text,
        .callback = NULL,
        .type = MENU_ITEM_VALUE_BOOL,
        .icon = NULL,
        .value = value,
        .submenu = NULL
    };
    return item;
}
