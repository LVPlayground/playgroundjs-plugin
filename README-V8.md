# V8 JavaScript engine
PlaygroundJS uses the [v8 JavaScript engine](https://code.google.com/p/v8/) to execute the JavaScript parts of the gamemode. This document describes which revision we're using, and how to build the libraries required for running the plugin.

## Current revision
PlaygroundJS tracks the master branch of v8, and is currently build against the following revision:

    be97f7d25c8a2da44fa888c833eedcd7291fd821
    "Version 8.3.129"
    Thursday, April 2nd, 2020

This is reflected in the [//src/v8](/src/v8) folder, which will load the given revision of the v8 JavaScript engine as a git submodule.

Updating the version of v8 we use involves changing the submodule to refer to the new revision, making sure everything builds on both Windows and Linux and checking in the new binaries, v8.dll and libv8.so, in [//bin](/bin).

# Building v8
If you want to change the plugin itself, or update the version of v8 that's being used, make sure that you've got [depot_tools installed](https://dev.chromium.org/developers/how-tos/install-depot-tools) and available in your PATH.

## First time compilation (Linux only)
If this is the first time you're building v8 on Linux, run the following commands to initialize your environment.

    $ git submodule update --init --recursive
    $ sudo yum install glibc-static

Depending on the distribution, [a more modern compiler](https://github.com/phpv8/v8js/wiki/Installing-on-CentOS-7-x64---PHP-7.3) may be necessary too.

## Common steps
The following preparatory steps have to be executed on all platforms. They check out v8 including all the dependencies specific to the platform.

    $ cd src/v8
    $ git pull --rebase
    $ git checkout 8.3.129
    $ gclient sync

## Building on Windows
On Windows, run the following command to create an MSVC-compatible binary:

    $ set DEPOT_TOOLS_WIN_TOOLCHAIN=0
    $ set GYP_MSVS_VERSION=2019
    $ set GYP_CHROMIUM_NO_ACTION=0
    $ gn gen --ide=vs2019 --args="target_cpu=\"x86\" is_component_build=true is_debug=false v8_use_external_startup_data=false v8_enable_i18n_support=false is_clang=false use_custom_libcxx=false" out.gn\x86.release
    $ ninja -C out.gn/x86.release v8 v8_libplatform

Browse to `src\v8\out.gn\x86.release` where you will find `v8.dll`, `v8_libbase.dll` and `v8_libplatform.dll` that have to be copied to the [//bin](/bin) directory.

Further, Boost will have to be built. After running the bootstrap, the following command is of use:

    $ b2.exe architecture=x86 --with-filesystem --with-system --with-regex --with-date_time

## Building on Linux
On Linux, the following commands should be used to compile v8 completely from the command line.

    $ gn gen --args='target_cpu="x86" is_component_build=true is_debug=false v8_use_external_startup_data=false v8_enable_i18n_support=false is_clang=false use_custom_libcxx=false' out.gn/x86.release
    $ ninja -C out.gn/x86.release v8 v8_libplatform

The [Makefile](src/Makefile) will automatically copy the required files to the appropriate directories.
