#include <gint/display.h>
#include <gint/keyboard.h>

int main(void)
{
	dclear(color_white);
	dtext(1, 1, "Sample fxSDK add-in.", C_BLACK, C_NONE);
	dupdate();

	getkey();
	return 1;
}
