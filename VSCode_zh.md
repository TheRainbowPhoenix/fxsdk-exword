# 如何在 Visual Studio Code 中获得代码提示

虽然目前您没有办法在电脑上运行程序，更不用说 debug 了，但是您仍然可以享受全方位的代码提示！这已经比卡西欧在 2007 年推出的那个 IDE 好多了。

## 下载

虽然这个软件很火，但有些人可能真的没听说过，先去 [Visual Studio Code 官网](https://code.visualstudio.com) 下载吧！

## 安装插件

首先安装来自微软的 [C/C++ 插件](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools)。或者直接在“扩展”中搜索 `C/C++` 也可以。

## 告诉插件真正要用的编译器

一般情况下，我们是开发在电脑上运行的程序。但是这下不一样了，需要使用 fxSDK 的跨平台编译器。

打开命令面板（CTRL/CMD + Shift + P），输入 `C/C++: Edit Configurations (UI)` 并按回车。

将下面的路径填入“编译器路径”一栏。

```
/home/你的用户名/.local/bin/sh-elf-gcc
```

譬如我的用户名为 `chenzhanming`，我就写：

```
/home/chenzhanming/.local/bin/sh-elf-gcc
```

## 看看效果

打开 `src/main.c` 文件，输入以下代码，看看代码提示是否有正常工作。

```c
#include <gint/display.h>
#include <gint/keyboard.h>

int main(void)
{
    // 用白色来清空屏幕
	dclear(C_WHITE);

    // 用黑色在左上角写上 Hello world!
	dtext(1, 1, C_BLACK, "Hello world!");
    
    // 在检测到下一次按键之前卡在这里。
    // 因为在卡西欧计算器上，程序结束之后将直接返回菜单，
    // 所以只有卡在这里不让它结束，才能看到程序运行结果。
	getkey();
	
    return 1;
}
```

如果一切正常的话你会发现，上方的 `C_WHITE` 的 `C_BLACK` 出现错误，因为目前编辑器不知道您是在给彩屏还是黑白屏的计算器写程序。如果您希望让自己的程序可以在两种屏幕上跑，则需要分别考虑两种情况。现在，姑且先让它在黑白屏上跑吧：

```c
#include <gint/display.h>
#include <gint/keyboard.h>

int main(void)
{
    // 如果是黑白屏幕
	#ifdef FX9860G
		// 用白色来清空屏幕
        dclear(C_WHITE);

        // 用黑色在左上角写上 Hello world!
        dtext(1, 1, C_BLACK, "Hello world!");
	#endif

	// 在检测到下一次按键之前卡在这里。
    // 因为在卡西欧计算器上，程序结束之后将直接返回菜单，
    // 所以只有卡在这里不让它结束，才能看到程序运行结果。
	getkey();
	
    return 1;
}
```

然后您会发现没有报错了。因为 gint 内核要求先定义计算器型号，才可以使用颜色。

如果您下定决心只在黑白屏或者彩色屏上写程序，那您就可以把这个定义放在程序的最顶端：

```c
#define FX9860G
#include <gint/display.h>
#include <gint/keyboard.h>

int main(void)
{
	// 用白色来清空屏幕
	dclear(C_WHITE);

	// 用黑色在左上角写上 Hello world!
	dtext(1, 1, C_BLACK, "Hello world!");

	// 在检测到下一次按键之前卡在这里。
    // 因为在卡西欧计算器上，程序结束之后将直接返回菜单，
    // 所以只有卡在这里不让它结束，才能看到程序运行结果。
	getkey();
	
    return 1;
}
```

这样同样也不会有报错。但是如果您实在是不想这么写，可以让告诉编译器，默认使用黑白屏。

请再一次用命令面板打开 `C/C++: Edit Configurations (UI)`。

在“编译器参数”一栏写下：

```
-D FX9860G
```

这样一来，最开始的那份代码也不会有报错了。唯一需要注意的是，在调用 `fxsdk` 来编译的时候，这个工具会主动告诉编译器计算器的型号是什么，所以在编译的时候不会报错。这里只是为了在开发的时候能够获取正确的代码提示罢了。

## 结语

好啦！您已经学会了如何用这个牛逼轰轰的内核来开发计算器程序了，期待您的作品！
