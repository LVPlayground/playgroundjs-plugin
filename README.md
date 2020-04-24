# PlaygroundJS
The canonical language for San Andreas: Multiplayer gamemodes gamemodes has always been [Pawn](http://www.compuphase.com/pawn/pawn.htm).

In the past nine years, Las Venturas Playground has accumulated over 90,000 lines of Pawn code spread over more than 425 files, written [their own Pawn compiler](https://sa-mp.nl/development/tools/precompiler/) and extended the syntax of the language to make it easier to use. Dozens of our staff members are deeply familiarized with the language.

While the language has served us well, it has significant limitations. Originally designed for use on embedded systems, Pawn scripts have a fixed memory footprint, and do therefore not have a heap. The absence of pointers necessitates extensive decision branches, and the amount of technical debt in our own code is depressing as well.

# JavaScript, and the v8 Engine
We considered many options in our search for the _magical realms with unicorns and rainbows_. A fresh start would be great, but is realistically unfeasible because of the required investment. Native code (C++) is great, but would significantly limit the number of developers we have.

Then there's JavaScript. Many developers touch it at least once, as it's the foundational language for Web development. It's easily accessible, got _gazillions_ of tutorials, highly performant and its fault tolerancy reduces the significance of errors.

We chose the [v8 engine](https://code.google.com/p/v8/) because of its performance and friendly C++ API.

# In addition, not instead
Las Venturas Playground does not have the development resources to rewrite the entire gamemode from scratch.

Therefore, JavaScript may be used _in addition_ of the existing Pawn gamemode, and not _instead_ of it. A powerful [event model](docs/events.md) enables all SA-MP callbacks to be used and all [native functions](docs/natives.md) are available from JavaScript as well.

# Dependencies
The PlaygroundJS plugin requires a series of dependencies to be available on the host. When compiling the plugin, both binaries and source must be available.

  * [Boost 1.72](https://www.boost.org/) or later.
  * [libmysql 5.7](https://dev.mysql.com/doc/refman/5.7/en/)
  * [OpenSSL 1.1.1g](https://www.openssl.org/source/) or later.

It should take a couple of hours to set this up on a Windows 10 machine with Visual Studio 2019.

# Further reading
This repository contains the source code of [the plugin](plugin/src) as well as the source code of our JavaScript-based [gamemode](javascript/). All code is available under a friendly [MIT license](LICENSE.md), although we do ask you to give us some attribution.

An [installation guide](INSTALL.md) is available for PlaygroundJS as well.

Read [the documentation](docs/), or continue to the [Las Venturas Playground website](https://sa-mp.nl/).