\page known_issues Known Issues
This page lists planned features, incomplete subsystems, and known bugs within the PowerBlocks SDK.

It is intended to help contributors understand priority areas and to help set expectations for users.

If you discover an issue not listed here or want to dive deeper into something already listed, please open a report on the 
[PowerBlocks GitHub Issues Page](https://github.com/rainbain/PowerBlocks/issues).

---

# Toolchain & Compiler

## PowerPC 750CL SIMD Support
The “paired-singles” SIMD instructions on the PowerPC 750CL are an IBM extension to the standard 750 core. 
LLVM’s PowerPC backend does not currently implement support for these instructions.

They are documented on: 
https://wiibrew.org/wiki/Paired_single

Past attempts to upstream support were made years ago, but the LLVM backend has changed significantly since then, so those patches are no longer compatible.

Long-term goals:
- Add paired-single support to LLVM
- Investigate whether auto-vectorization for paired-singles is feasible

---

# Video Interface (VI)

## Confirm Video Modes
PowerBlocks currently provides many video modes based on prior homebrew conventions.  
Some of these may not be necessary, and some may not work correctly on all consoles or display setups.

## Video Blackout During Initialization
The video subsystem does not currently black out the screen during initialization.  
This results in visible glitching or artifacts during boot. 
A proper blackout would prevent this.

---

# GX

## Missing Features
- Display lists  
- TEV color swapping  
- Indirect texturing  
- Texture LOD support  

---

# Bluetooth

## No double buffering on HCI.
HCI receive events from bluetooth endpoint likely need to be double buffered. Not doing this causes it to drop HCI events.

# Missing L2CAP Channel Disconnect Event Handler
This code does not handle when a L2CAP channel attempts to disconnect. HCI disconnects are respect to be the master disconnect all.

---

# Wii Remotes (Wiimotes)

## Missing Extensions
- MotionPlus  
- Classic Controller Pro  
- Drawsome Graphics Tablet  
- Guitar Hero Guitar  
- Guitar Hero Drums  
- DJ Hero Turntable  
- Taiko no Tatsujin TaTaCon drum controller  
- uDraw GameTablet  
- Densha de GO! Shinkansen Controller  
- Wii Balance Board  

---

# File System

## SD Card Hot-Swapping
SD card mounting and unmounting has not been fully tested.  
Hot-swap handling will need to be implemented cleanly.

## Missing POSIX Functions
Many standard POSIX filesystem functions are only partially implemented or untested.

---

# Bugs

## rand() Crash
Using `rand()` currently results in a crash.  
The cause has not yet been investigated.

## framebuffer_fill_rgba() Incorrect Fill Colors
Drawing small rectangles (e.g., 5×5) sometimes results in incorrect or “random” colors.  
This appears related to UV or pitch calculations when dimensions are not multiples of two.

## Wiimote Example Crash on Startup
The Wiimote example occasionally crashes with a black screen during early initialization.  
A system reboot consistently fixes the issue, suggesting that the cause is uninitialized or stale memory rather than the Wiimote code itself.
