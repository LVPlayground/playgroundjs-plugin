# V8 JavaScript engine
PlaygroundJS uses the [v8 JavaScript engine](https://code.google.com/p/v8/) to execute the JavaScript parts of the gamemode. This document describes which revision we're using, and how to build the libraries required for running the plugin.

## Current revision
PlaygroundJS tracks the master branch of v8, and is currently build against the following revision:

    0ad9b9e523c82320dc6fa5483d2fae56ea9cf9b4
    "PPC: [builtins] Add support for NewTarget to Execution::New."
    Tuesday, September 22nd, 2015

This is reflected in the [//plugin/src/v8](/plugin/src/v8) folder, which will load the given revision of the v8 JavaScript engine as a git submodule.

Updating the version of v8 we use involves changing the submodule to refer to the new revision, making sure everything builds on both Windows and Linux and checking in the new binaries, v8.dll and v8.so, in [//plugin/bin](/plugin/bin).

# Building v8
If you want to change the plugin itself, or update the version of v8 that's being used, make sure that you've got [depot_tools installed](https://dev.chromium.org/developers/how-tos/install-depot-tools) and available in your PATH.

## Common steps
The following preparatory steps have to be executed on all platforms.

    $ git submodule update
    $ cd plugin/src/v8
    $ git clone https://chromium.googlesource.com/external/gyp build/gyp
    $ git clone https://chromium.googlesource.com/external/googletest.git testing/gtest
    $ git clone https://chromium.googlesource.com/external/googlemock.git testing/gmock
    $ svn co -q http://src.chromium.org/svn/trunk/tools/third_party/python_26 third_party/python_26

## Building on Windows
On Windows, a dependency on cygwin also exists because of the js2c generator.

    $ git clone https://chromium.googlesource.com/chromium/deps/cygwin.git third_party/cygwin
    $ python build/gyp_v8 -Dtarget_arch=ia32 -Dcomponent=shared_library -Dv8_use_snapshot=0 -Dv8_use_external_startup_data=0 -Dv8_enable_i18n_support=0

Open `plugin\src\v8\build\all.sln` in the Visual Studio version you're using to build the plugin, and build the `all` target in release mode for x86.

Browse to `plugin\src\v8\build\Release`, where you will find `v8.dll` that has to be copied to the [//plugin/bin](/plugin/bin) directory.

## Building on Linux
TODO