# V8 JavaScript engine
PlaygroundJS uses the [v8 JavaScript engine](https://code.google.com/p/v8/) to execute the JavaScript parts of the gamemode. This document describes which revision we're using, and how to build the libraries required for running the plugin.

## Current revision
PlaygroundJS tracks the master branch of v8, and is currently build against the following revision:

    bef2fbcc690784f554d3b688da02b9efc38fd7df
    "[cleanup] Use NewVariableProxy() instead of NewUnresolved where possible"
    Friday, October 9th, 2015

This is reflected in the [//src/v8](/src/v8) folder, which will load the given revision of the v8 JavaScript engine as a git submodule.

Updating the version of v8 we use involves changing the submodule to refer to the new revision, making sure everything builds on both Windows and Linux and checking in the new binaries, v8.dll and libv8.so, in [//bin](/bin).

# Building v8
If you want to change the plugin itself, or update the version of v8 that's being used, make sure that you've got [depot_tools installed](https://dev.chromium.org/developers/how-tos/install-depot-tools) and available in your PATH.

## Common steps
The following preparatory steps have to be executed on all platforms. They check out v8 including all the dependencies specific to the platform.

    $ git submodule update
    $ cd src/v8
    $ gclient sync

## Building on Windows
On Windows, run the following command to create the MSVC project files:

    $ python build/gyp_v8 -Dtarget_arch=ia32 -Dcomponent=shared_library -Dv8_use_snapshot=0 -Dv8_use_external_startup_data=0 -Dv8_enable_i18n_support=0

Then open `src\v8\build\all.sln` in the Visual Studio version you're using to build the plugin, and build the `all` target in release mode for x86.

Browse to `src\v8\build\Release` where you will find `v8.dll` that has to be copied to the [//bin](/bin) directory.

## Building on Linux
On Linux, the following commands should be used to compile v8 completely from the command line.

    $ make -j32 ia32.release component=shared_library snapshot=off i18nsupport=off

Browse to `src/v8/out/ia32.release/lib.target/` where you will find `libv8.so` that has to be copied to the [//bin](/bin) directory.
