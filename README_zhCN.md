# fxSDK

*Planète Casio 上的一个帖子：[fxSDK 是另外一套开发计算器程序的工具链（法语帖子）](https://www.planet-casio.com/Fr/forums/topic13164-last-fxsdk-un-sdk-alternatif-pour-ecrire-des-add-ins.html)*

fxSDK 是一套开发卡西欧图形计算器里的程序的工具，里面提供了命令行工具、构建系统以及一个跨平台编译器，用来开发程序以及库。这套工具需要和 [gint 内核](/Lephenixnoir/gint) 一起使用，因为这个内核提供的运行时非常之厉害。

这个仓库只提供了 SDK 的命令行工具。如果想要构建程序，需要一些其他的组件，比如[跨平台编译器](/Lephenixnoir/sh-elf-gcc)还有 [gint 内核](/Lephenixnoir/gint)。更多信息请移步至[如何安装]()。

fxSDK 以 [MIT 协议](LICENSE)发布。

## 支持的机型及其兼容性

**计算器**

fxSDK，或者说 gint，支持卡西欧 fx-9860G 系列几乎所有的型号，包括：

* ![](https://www.planet-casio.com/images/icones/calc/g85.png)（部分支持）基于SH3 的 fx-9860G、fx-9860G SD、以及类似运行 1.xx 版本操作系统的型号，还有 fx-9860G SDK 里的模拟器。
* ![](https://www.planet-casio.com/images/icones/calc/g95.png) 基于 SH3 的 fx-9750G、fx-9860G II、fx-9860G II SD，以及类似运行 2.xx 版本操作系统的型号。
* ![](https://www.planet-casio.com/images/icones/calc/g75+.png) 所有基于 SH4 且运行操作系统 2.xx 的型号，包括 fx-9750G II 与 SH4 的 fx-9860G II。
* ![](https://www.planet-casio.com/images/icones/calc/g35+e2.png) fx-9750G III 与 fx-9860G III。

也支持卡西欧 fx-CG 系列的计算器：

* ![](https://www.planet-casio.com/images/icones/calc/cg20.png) 经典款 fx-CG 10/20。
* ![](https://www.planet-casio.com/images/icones/calc/g90+e.png) fx-CG 50。

**操作系统**

* Linux 有官方支持。只要装好了依赖，任何的发行版都可以运行。
* macOS 由于缺少测试，需要进行一些小小的调整才可以运行。
* Windows 因为有 WSL（Linux 子系统），所以也有官方支持。然而 Windows 本身并不受支持，但如果您想的话可以尝试贡献。

**编程语言**

* C 语言：支持最新版 GCC，目前是 C11 或者 C2X。标准库 [fxlibc](https://gitea.planet-casio.com/Vhex-Kernel-Core/fxlibc/) 是我们自行构建的，大致遵循 C99。使用其他的标准库也是可以的。
* C++：支持最新版 GCC，目前是 C++20 或者 C++23。标准库 [libstdc++](https://gcc.gnu.org/onlinedocs/libstdc++/) 也是最新的。
* 汇编语言：卡西欧计算器的处理器为 SuperH 架构，其中数 SH4AL-DSP 最多。二进制工具里面提供的汇编器可供使用。
* 还想要其他的语言？只要能编译到 C 就行，比如 [fxtran for Fortran](https://www.planet-casio.com/Fr/forums/topic17064-1-fxtran-codez-en-fortran-pour-votre-casio.html)。其他的 GCC 支持的前端语言也可以，比如 [the D compiler builds](https://www.planet-casio.com/Fr/forums/topic17037-last-omegafail-dlang-et-gint.html)。对于那些基于 LLVM 的语言（比如 Rust）不在考虑范围内，因为 LLVM 没有 SuperH 后端。

**构建系统**

fxSDK 主要使用 CMake 来构建程序和库。有个老的 Makefile 模板仍然可以用，而且命令行工具仍有官方支持，但是在更新的时候就得手动修改您自己的构建系统了。

## 如何安装

这里说明的安装方法不仅仅适用这个仓库，还包括 fxSDK 里的所有东西。

**方法一：使用 GiteaPC（新手推荐，虽然现在不叫这个了）**

[GiteaPC](https://gitea.planet-casio.com/Lephenixnoir/GiteaPC) 是一个类似包管理器一样的东西，专门用来下载、构建及安装来自 Planète Casio 仓库的东西。整个过程是自动的，所以推荐在安装 fxSDK 的时候使用。

**方法二：在 Arch Linux 发行版里使用 AUR**

[Dark Storm](https://www.planet-casio.com/Fr/compte/voir_profil.php?membre=Dark%20Storm) 为 Arch Linux 维护了一个叫 [MiddleArch](https://www.planet-casio.com/Fr/forums/topic16790-1-middlearch-un-depot-communautaire.html) 的仓库，包含了最新版本的 fxSDK。

**方法三：手动构建（如果您是专家）**

You can build the fxSDK and its tools manually by following the instructions in each README file. Please refer to the GiteaPC tutorial for a list of what to install in what order. As a warning: there is quite a lot of stuff (SDK tools, the cross-compiler, a libm, a libc, the libstdc++, the kernel, user libraries and then some) so expect to spend some time installing and updating everything.

## 命令行工具

当你用 fxSDK 开发程序的时候，你主要会使用命令行工具和 fxSDK 的构建系统。先来看看命令行工具吧。当你不传任何的参数直接调用工具的时候会显示帮助。例如 `fxsdk` 或 `fxgxa`。

*Note: A tool called `fxos` used to live here and has now moved to [its own repository](/Lephenixnoir/fxos).*

使用 `fxsdk` 来**管理项目**

你可以用 `fxsdk new` 来创建一个新项目，并指定名字。本例为“MyAddin”：

```sh
fxsdk new MyAddin
```

然后进入这个文件夹：

```bash
cd MyAddin
```

进入新创建的文件夹之后，就可以用 `fxsdk build-*` 来构建程序：

```bash
# 构建 fx-9860G 系列计算器的程序（后缀 .g1a）：
fxsdk build-fx
# 构建 fx-CG 系列计算器的程序（后缀 .g3a）：
fxsdk build-cg
```

然后你就可以把这个程序用你喜欢的方式发到计算器上了。但是这里有些更简单的方法：

* `fxsdk send-fx` 会使用 [p7](/cake/p7utils) 来发送 g1a 程序，就像 FA-124 和 xfer9860 那样。除了 fx-975G0G III 和 fx-9860G III，所有 fx-9860G 系列的型号都可以用。
* `fxsdk send-cg` 会使用 UDisks2 来发送 g3a 程序，其实就相当与您在文件资源管理器里面操作一样。下面具体有写 fxlink 做了什么。

`fxsdk path` 会告诉你 SDK 里面的一些重要文件（主要是跨平台编译器）的位置。

使用 `fxgxa` 来**生成 G1A 和 G3A 文件**

`fxgxa` 是一个多功能的 g1a 与 g3a 文件编辑器，可以创建、编辑与导出程序的头部。实际上，在运行 `fxsdk build-*` 这个命令的时候，您也间接调用了 `fxgxa`。所以这个只有在您想要查看已经编译好的程序的内容的时候才会用得到。

这可以用来导出 PNG 格式的图标、执行校验和、修复损坏的头部以及导出程序的其他信息。主要是这些命令：

* `fxgxa --g1a|--g3a`：生成 g1a 或 g3a 文件
* `fxgxa -e`：编辑 g1a 或 g3a 文件
* `fxgxa -d`：提取元数据、校验和，以及图标
* `fxgxa -r`：修复破损文件的控制字节与校验和
* `fxgxa -x`：以 PNG 格式提取图标

`fxgxa` 有另一个名字 `fxg1a`。这是为了兼容 2.7 版本以前的 fxSDK，因为在那时还不能生成 g3a 程序。

使用 `fxconv` 来**转换素材格式**

`fxconv` is a programmable asset converter that converts images, fonts and other common asset types into data structures usable directly in add-ins. The built-in formats include gint images and fonts, [libimg](/Lephenixnoir/libimg) images, and binary blobs.

Projects can extend the support to custom types for maps, dialogs, GUI descriptions, or other application-specific assets. Extensions to `fxconv` are implemented in Python within the project.

`fxconv` is tightly integrated into the build system. Normally you declare assets in a `CMakeLists.txt` file, set their parameters in an `fxconv-metadata.txt` file, and then let the build system do the magic.

TODO: Link to gint tutorials or better explain how to use fxconv.

使用 `fxlink` 来**进行 USB 通信**

`fxlink` is a USB communication tool that can be used to send files to calculators as well as to communicate with gint's USB driver from an add-in. The tool is still it its early stages but already offers two useful features.

Note: `fxlink` does not work within WSL machines on Windows, see [this bug](https://github.com/Microsoft/WSL/issues/2195).

The first feature is interactive communication with add-ins using libusb. This allows add-ins to send text, screenshots, and video captures of their output to a computer in real-time.

The second feature is sending files to fx-CG and G-III calculators (the ones that behave like USB drives) using UDisks2. `fxlink` can mount the calculators, copy files and unmount them from the command-line without root access.

## 真的，爽！但是我想要代码提示！

[如何在 Visual Studio Code 中获得代码提示](/src/branch/master/VSCode_zhCN.md)

## 构建系统

**使用 CMake**

The official build system is CMake since fxSDK 2.3. When creating a new project, a default `CMakeLists.txt` is generated. There are few deviations from standard CMake practices; refer to a CMake tutorial for general explanations (there is [an fxSDK-specific one in French on Planète Casio](https://www.planet-casio.com/Fr/forums/topic16647-1-tutoriel-compiler-des-add-ins-avec-cmake-fxsdk.html)). The differences are explained below.

Because we are using a cross-compiler, we can't just call `cmake` to configure the project; extra parameters are needed. The `fxsdk build-*` commands call CMake for you with the correct parameters.

The fxSDK provides [a couple of modules](fxsdk/cmake), including:

* [`FX9860G.cmake`](fxsdk/cmake/FX9860G.cmake) and [`FXCG50.cmake`](fxsdk/cmake/FXCG50.cmake) that are loaded automatically by `fxsdk build-fx` and `fxsdk build-cg` respectively. These are one of the reasons why we don't call `cmake` directly. Anything defined here is available to your `CMakeLists.txt`, which includes a number of variables called `FXSDK_*` to give you information on the target and install.
* [`Fxconv.cmake`](fxsdk/cmake/Fxconv.cmake) which provides functions to use fxconv. `fxconv_declare_assets(... WITH_METADATA)` will mark source files as assets to be converted with fxconv, and `fxconv_declare_converters(...)` declares Python modules containing custom conversion functions.
* [`GenerateG1A.cmake`](fxsdk/cmake/GenerateG1A.cmake) and [`GenerateG3A.cmake`](fxsdk/cmake/GenerateG3A.cmake) wrap `fxgxa` and allow you to generate g1a/g3a files for the add-in. The default `CMakeLists.txt` shows how to use them.
* [`FindSimpleLibrary.cmake`](fxsdk/cmake/FindSimpleLibrary.cmake) and [`GitVersionNumber.cmake`](fxsdk/cmake/GitVersionNumber.cmake) are general utilities for libraries. See the [Lephenixnoir/Template-gint-library](https://gitea.planet-casio.com/Lephenixnoir/Template-gint-library) repository for an example of building a library with the fxSDK.

**只使用 Makefile**

The original Makefile used to build add-ins is still available. A Makefile-based project can be created with the `--makefile` option of `fxsdk new`. However that Makefile is rarely tested thus occasionally out-of-date, and in general requires you to maintain it through fxSDK updates. It is only advised to use it if you're experienced with make.

## 手动构建

下面是手动安装 fxSDK 的步骤。一开始，您应该先安装本仓库。如果您之前已经安装过 fxSDK 或者跨平台编译器之类的，为了避免冲突，您应该先把它们删掉之后再来安装。

这个仓库的依赖:

* CMake
* libpng ≥ 1.6
* Python ≥ 3.7（3.6 有可能也可以）
* Python 3 的 Pillow 库
* libusb 1.0
* UDisks2 库（在构建工具链的时候如果没用就不用）

When configuring, you should set an install prefix that you have write access to. I suggest `$HOME/.local`. Note that the cross-compiler *must* be later installed in the path printed by `fxsdk path sysroot`, which is within said prefix.

* Use `-DCMAKE_INSTALL_PREFIX=...` to change the install folder;
* Use `-DFXLINK_DISABLE_UDISKS2=1` to disable UDisks2 support in `fxlink`, if you don't have UDisks2 or you're using WSL.

```bash
% cmake -B build [OPTIONS...]
% make -C build
% make -C build install
```

You can then proceed to install the cross-compiler. If in doubt about the order in which you need to install repositories, refer to the [GiteaPC README](https://gitea.planet-casio.com/Lephenixnoir/GiteaPC) or check the `giteapc.make` files of each repository, where dependencies are listed.
