/* Standalone EX-word LCD demo: no ROM calls, updater code, or flash writes. */
typedef unsigned char u8;
typedef unsigned short u16;
#define PRDR (*(volatile u8 *)0xa405013c)
#define LCD  (*(volatile u16 *)0xb4000000)

static inline void sync_bus(void) { __asm__ volatile ("synco" ::: "memory"); }
static void command(u8 value)
{
    PRDR = PRDR & (u8)~0x10;
    sync_bus();
    LCD = value;
    sync_bus();
    PRDR = PRDR | 0x10;
    sync_bus();
}
static void data(u16 value) { LCD = value; sync_bus(); }

void main(void)
{
    /* D6600: 528 columns, 320 visible rows, controller row offset 40. */
    command(0x2a);
    data(0); data(0); data(2); data(15);
    command(0x2b);
    data(0); data(40); data(1); data(103);
    command(0x2c);
    for (unsigned y = 0; y < 320; ++y) {
        for (unsigned x = 0; x < 528; ++x) {
            unsigned half = (y - 64) >> 1;
            u16 color = (y >= 64 && y <= 256 &&
                         x >= 264 - half && x <= 264 + half) ? 0xffff : 0;
            data(color);
        }
    }
    command(0x29);
    for (;;) __asm__ volatile ("nop");
}
