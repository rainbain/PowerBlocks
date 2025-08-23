# Teapot (GX Test)
This demo is designed to test a good range of the functions of
GX.

To build it first export the sdk.
```
. ./export.sh
```

Build the code:
```
mkdir build
cd build
cmake ..
ninja
```

From here a .elf file is provided. You can convert this to .dol with an external tool or use the ELF directly.

It is recommended to launch directly through Homebrew Channel so that the correct system environment is set up.