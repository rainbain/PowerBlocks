# Contributing
First I want to say thank you for contributing!

This document outlines how to contribute to PowerBlocks.

The general hope is to make PowerBlocks a strong and powerful SDK for the Wii, so contributions are much welcome.

## Code of Conduct
All contributes follow under the code of conduct. Its nothing too crazy but do check it out in the `CODE_OF_CONDUCT.md` file.

## How can I contribute?
Contributes can be as simple as fixing bug request, making a feature more extendable, or even adding your own block!

Just fork it, make your changes, and submit a PR. I will do my best to review it, provide relevant feedback, and hopefully merge it.

Testing is also much appreciated, just letting me know what works on hardware and does not.

Again think you for your contribution!


## Project Structure
PowerBlocks is organized into blocks.

The core block is everything the SDK needs to operate as a SDK. This includes hardware access and some coverage of all hardware multi media on the system like video and audio.

Blocks add features every users will need, but are tools that should be implemented within the SDK. Like libraries, interfacing with obscure hardware addons, and more.


## Style Guide
### Header Files
Each header file has a Doxygen compatible comment block. This is expected on the core block, as well as additional blocks.

Most functions and any other relevant parts to using the header should also have the same documentation.

### Naming Convention
Functions are named like `module_action_description` and `ios_settings_initialize.`

Definitions and constants are named with `ALL_CAPS_WITH_UNDERSCORES`. like:
```c
static const char* NAME = "VALUE";
```

### Formatting
 * 4 spaces (not tabs.)
 * Opening braces on the same line.
 * Keep lines under ~100 characters.
 * At least 1 blank line between functions.

 ## Testing
 Make sure to include the test of your contributions when it changes the behavior of something.

 This preferably needs to be on actual hardware, and if you can, for a long period of time.

 The hardware tends to be very weird and emulator is almost never enough.

 I can test it too. Just don't brink my Wii lol.