# Using fxSDK with CLion

CLion is a powerful tool for developing C/C++ projects,
allowing complex code inspection and refactoring,
helping you catch bugs before running it on your calculator.

## Download CLion

Download CLion from [JetBrain's website](https://www.jetbrains.com/clion)
for free for non-commercial projects!

Open your project's folder with CLion.

## Add Toolchain

Open **Settings**.

Select **Toolchain**.

Click `+` in the toolchain list.

+ Select **System** if fxSDK is installed natively.
+ Select **WSL** if using Windows subsystem Linux.

Fill in the blanks, replace `~` with your home directory.

+ Set **Build Tool** as `/usr/bin/cmake`.
+ Set **C Compiler** as `~/.local/bin/sh-elf-gcc`.
+ Set **C++ Compiler** as `~/.local/bin/sh-elf-g++`.
+ Set **Debugger** as `~/.local/bin/sh-elf-gdb`.

## Add CMake Configuration

Open **Settings**.

Select **CMake**.

Click `+` in the configuration list.

Set **Toolchain** as the one we created.

Set **CMake Options** as the following, replace `~` with your home directory.

```sh
-DCMAKE_MODULE_PATH=~/.local/lib/cmake/fxsdk
-DFXSDK_CMAKE_MODULE_PATH=~/.local/lib/cmake/fxsdk
-DCMAKE_TOOLCHAIN_FILE=~/.local/lib/cmake/fxsdk/FX9860G.cmake
```

For other devices, you can replace `FX9860G` with `FXCG50`, `FX9860G_G3A`, or `FXCP`.
Check [fxsdk.sh](./fxsdk/fxsdk.sh) for details.

Set **Build Options** as the following.

```sh
-- -C .
```

Set **Environment** as the following, replace `~` with your home directory.

```sh
PREFIX=~/.local;PATH=~/.local/bin
```

## Almost there...

Right click `CMakeLists.txt` and press **Reload CMake Project**.
You should do this when `CMakeLists.txt` changes.

Press the **hammer** button to build the executable.

Now you can enjoy the powerful functionalities provided by CLion.
You may also want to create a simple native C project to familiarise yourself with this IDE.
