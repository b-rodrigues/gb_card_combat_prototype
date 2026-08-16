#include "ui.h"
#include "menu.h"

void menu_draw_frame(const MenuFrame *frame)
{
    if (!frame) return;
    ui_clear_screen();
    menu_draw_centered(frame->title_row, frame->title);
    ui_draw_hline((uint8_t)(frame->title_row + 1), '-');
}



void menu_draw_centered(uint8_t y, const char *text)
{
    uint8_t len = 0;
    uint8_t x;
    if (!text) return;
    while (text[len] != '\0' && len < MENU_WIDTH) len++;
    x = (uint8_t)((MENU_WIDTH - len) >> 1);   /* 0 when len == MENU_WIDTH */
    ui_draw_text_line(x, y, text, MENU_WIDTH);
}
