#include <gint/display.h>
#include <gint/keyboard.h>

int main(void)
{
	dclear(color_white);
	dtext(4, 4, "Sample fxSDK add-in.", color_black, color_none);
	dupdate();

	getkey();
	return 1;
}
