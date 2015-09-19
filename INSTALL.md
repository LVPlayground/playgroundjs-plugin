# Installing PlaygroundJS
Preparing your system to work with PlaygroundJS is not entirely trivial. This guide should hopefully provide some insight in the requirements. If you've got further questions, please [file an issue](https://github.com/RussellLVP/playgroundjs/issues/new)!

This guide assumes that you already have a working Las Venturas Playground checkout.

## Create symbolic links
The easiest way to use PlaygroundJS is to create symbolic links to the GitHub checkout. This enables you to update to the latest version simply by synchronizing your checkout.

### Windows
Run a new command prompt as an administrator(!), and change to the directory where the SA-MP server lives. Run the following commands:

```
mklink /d javascript "..\checkout\javascript\"
mklink plugins\playground.dll "..\..\checkout\plugin\bin\playground.dll"
mklink callbacks.txt "..\checkout\plugin\bin\callbacks.txt"
mklink v8.dll "..\checkout\plugin\bin\v8.dll"
```

### Linux
_TODO: Write the Linux part of this guide._
