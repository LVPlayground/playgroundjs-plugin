# V8 JavaScript engine
PlaygroundJS uses the [v8 JavaScript engine](https://code.google.com/p/v8/) to execute the JavaScript parts of the gamemode. This document describes which revision we're using, and how to build the libraries required for running the plugin.

## Current revision
PlaygroundJS tracks the master branch of v8, and is currently build against the following revision:

    d840ed11d9fa9f38676e6edcee93ed749afb284d
    "Revert of [build] Introduce an embedder version string (...)"
    Saturday, January 21st, 2017

This is reflected in the [//src/v8](/src/v8) folder, which will load the given revision of the v8 JavaScript engine as a git submodule.

Updating the version of v8 we use involves changing the submodule to refer to the new revision, making sure everything builds on both Windows and Linux and checking in the new binaries, v8.dll and libv8.so, in [//bin](/bin).

# Building v8
If you want to change the plugin itself, or update the version of v8 that's being used, make sure that you've got [depot_tools installed](https://dev.chromium.org/developers/how-tos/install-depot-tools) and available in your PATH.

## Common steps
The following preparatory steps have to be executed on all platforms. They check out v8 including all the dependencies specific to the platform.

    $ cd src/v8
    $ git pull --rebase
    $ git checkout origin/master
    $ gclient sync

## Building on Windows
On Windows, run the following command to create the MSVC project files:

    $ $env:DEPOT_TOOLS_WIN_TOOLCHAIN = 0
    $ $env:GYP_MSVS_VERSION = 2015
    $ $env:GYP_CHROMIUM_NO_ACTION = 0
    $ gn gen --ide=vs2015 --args='target_cpu=\"x86\" is_component_build=true is_debug=false v8_use_snapshot=false v8_use_external_startup_data=false v8_enable_i18n_support=false' out.gn/x86.release
    $ ninja -C out.gn/x86.release v8 v8_libplatform

Browse to `src\v8\out.gn\x86.release` where you will find `v8.dll`, `v8_libbase.dll` and `v8_libplatform.dll` that have to be copied to the [//bin](/bin) directory.

## Building on Linux
On Linux, the following commands should be used to compile v8 completely from the command line.

    $ gn gen --args='target_cpu="x86" is_component_build=true is_debug=false v8_use_snapshot=false v8_use_external_startup_data=false v8_enable_i18n_support=false' out.gn/x86.release
    $ ninja -C out.gn/x86.release v8 v8_libplatform

Browse to `src/v8/out/ia32.release/lib.target/` where you will find `libv8.so` that has to be copied to the [//bin](/bin) directory.
