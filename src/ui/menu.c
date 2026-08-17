#include "ui.h"
#include "menu.h"

void menu_draw_frame(const char *title)
{
    uint8_t len = 0;
    if (!title) return;
    ui_clear_screen();
    while (title[len] != '\0' && len < MENU_WIDTH) len++;
    ui_draw_text_line((uint8_t)((MENU_WIDTH - len) >> 1), 0, title, MENU_WIDTH);
    ui_draw_hline(1, '-');
}

void menu_draw_centered(uint8_t y, const char *text)
{
    uint8_t len = 0;
    if (!text) return;
    while (text[len] != '\0' && len < MENU_WIDTH) len++;
    ui_draw_text_line((uint8_t)((MENU_WIDTH - len) >> 1), y, text, MENU_WIDTH);
}
