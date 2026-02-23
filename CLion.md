# Using fxSDK with CLion

CLion is a powerful tool for developing C/C++ projects,
allowing complex code inspection and refactoring,
helping you catch bugs before running it on your calculator.

## Download CLion

Download CLion from [JetBrains' website](https://www.jetbrains.com/clion)
for free for non-commercial projects!

Open your project's folder with CLion.

## Add Toolchain

Open **Settings**.

Select **Toolchain**.

Click `+` in the toolchain list.

+ Select **System** if fxSDK is installed natively.
+ Select **WSL** if using Windows subsystem Linux.

Fill in the blanks:

+ Set **Build Tool** as `/usr/bin/cmake`.
+ Set **C Compiler** as `~/.local/bin/sh-elf-gcc`.
+ Set **C++ Compiler** as `~/.local/bin/sh-elf-g++`.
+ Set **Debugger** as `~/.local/bin/sh-elf-gdb`.

## Add CMake Configuration

Open **Settings**.

Select **CMake**.

Click `+` in the configuration list.

Set **Toolchain** as the one we created.

Set **CMake Options** as the following:

```sh
-DCMAKE_C_FLAGS=-nostdlib
-DCMAKE_CXX_FLAGS=-nostdlib
-DCMAKE_MODULE_PATH=~/.local/lib/cmake/fxsdk
-DFXSDK_CMAKE_MODULE_PATH=~/.local/lib/cmake/fxsdk
-DCMAKE_TOOLCHAIN_FILE=~/.local/lib/cmake/fxsdk/FX9860G.cmake
```

For other devices, you can replace `FX9860G` with
`FXCG50`, `FX9860G_G3A`, or `FXCP`.
Check [fxsdk.sh](./fxsdk/fxsdk.sh) for details.

> You may remove these two CMake options after you finished setting up the environment,
> as they're for bypassing CLion's check.
> ```sh
> -DCMAKE_C_FLAGS=-nostdlib
> -DCMAKE_CXX_FLAGS=-nostdlib
> ```

Set **Build Options** as the following.

```sh
-- -C .
```

Set **Environment** as the following.

```sh
PREFIX=~/.local;PATH=~/.local/bin
```

If you want CLion to reload CMake automatically when you change CMakeLists.txt, tick "**Reload CMake project on editing CMakeLists.txt or other CMake configuration files**".

## Almost there...

Right click `CMakeLists.txt` and press **Reload CMake Project**. (Only do this if you didn't tick the checkbox mentioned above.)

Now you can trigger the build by either:

+ Pressing **Build** from the top menu and then press **Build Project** to build the executable.
+ Pressing the **hammer** button.
+ Executing `fxsdk build-*` from the shell.

Enjoy!
You may also want to create a simple native C project to
familiarise yourself with this IDE.
