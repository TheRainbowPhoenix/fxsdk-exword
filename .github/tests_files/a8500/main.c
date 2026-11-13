#include <graphics/drawing.h>
#include <graphics/color.h>
#include <graphics/text.h>
#include <graphics/lcdc.h>
#include <sh4a/input/keypad.h>

#define SCREEN_WIDTH 528
#define SCREEN_HEIGHT 320

int main(void)
{
    struct font *fnt = get_font();
    set_pen(create_rgb16(0, 0, 0));
    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    set_pen(create_rgb16(255, 255, 255));

    render_text(
        (SCREEN_WIDTH - (sizeof "FXTST" - 1) * fnt->width) / 2,
        SCREEN_HEIGHT / 2 - fnt->height,
        "FXTST");
    render_text(
        (SCREEN_WIDTH - (sizeof "FxSDK bundle test!" - 1) * fnt->width) / 2,
        SCREEN_HEIGHT / 2 + fnt->height,
        "FxSDK bundle test!");

    lcdc_copy_vram();
    for(;;) {
        keypad_read();
        if(get_key_state(KEY_POWER) || get_key_state(KEY_BACK))
            return -2;
    }
}
