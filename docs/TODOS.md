# TODOs
This is a more in-depth version of the TODOs from what is in the README.md.

This one is for parts that have some progress, but are missing features.
This is to track those missing features.

Feel free to add them as they are discovered.

## Compiler / ToolChain
1. Assembler Support for SIMD
2. Auto Vectorization of SIMD

## Video Interface
1. Fix the unclean startup. (Black Out Screen)
2. Are more video modes really needed?

## GX
1. Add Display List
2. Add TEV color swapping.
3. Add Indirect Texturing
4. Add Texture LODs

## FileSystem
1. SD card mounting/unmounting may not be pretty. Ive not tried it
2. Add the posix filesystem functions.

## Bugs
1. Fix rand() function.
2. Fix framebuffer fill function. When drawing 5x5 red rects for example it fails up update UV values correctly and makes colors.
3. Uninitialized memory? in wiimote example. The wiimote example sometimes just fails with a black screen. Seems to fix if you reboot the system, so its likely uninitialized memory