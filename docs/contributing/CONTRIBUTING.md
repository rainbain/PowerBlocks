\page contributing Contributing

First I want to say thank you for contributing!

This page provides information to help you contribute to PowerBlocks.

Our goal is to make PowerBlocks a robust and flexible SDK for Wii development, and contributions are always welcome.

## Code of Conduct
All contributors fall under the \ref code_of_conduct "Code of Conduct".  
Please make sure to read it before contributing.

## How can I contribute?
Contributions can be as simple as fixing a bug, improving an API, or adding a new block.

Fork the repository, make your changes, and submit a pull request.  
I will review it, provide feedback if necessary, and merge it when ready.

Testing is also much appreciated. Just letting me know what works on hardware and what does not is very helpful to provide a clean experience to users and developers.

Again, thank you for contributing!

# Project Structure
PowerBlocks is organized into blocks.

The core block contains everything required for the SDK to function.  
It includes hardware abstraction layers and fundamental systems used by other blocks.

Additional blocks provide optional features such as Wii Remote support, filesystem libraries, and more.  
These help developers without forcing a single implementation style.

Documentation is generated with Doxygen from the [docs folder](https://github.com/rainbain/PowerBlocks/tree/main/docs). Improvements to existing documentation and contributions covering Wii hardware or SDK components are greatly appreciated.

# Style Guide
### Header Files
Each header file has a Doxygen compatible comment block. This is expected on the core block, as well as additional blocks.

Headers should be expected to have doxygen documentation. If a header is a core component of an API or block, it should also have its own API documentation.

### Naming Convention
Any names within header files first start with the name of the header. This provides namespacing to prevent names from getting mixed up.

Acronyms or abbreviations are avoided unless its a generally well understood shortened name like `cmd`, or is a standardized name, like `l2cap`.

The hope is for naming to be direct and consistent across PowerBlocks to make the code base easy to search through and manage.

Functions and exported variables follow `snake_case`, where all letters are lowercase and words are separated by underscores

```c
// powerblocks/core/bluetooth/hci.h

extern hci_acl_packet_t hci_acl_packet_out ALIGN(32) MEM2;

...

int hci_send_acl(uint16_t handle, hci_acl_packet_boundary_flag_t pb, hci_acl_packet_broadcast_flag_t bc, uint16_t length);
```

Definitions and constants are named with `ALL_CAPS_WITH_UNDERSCORES`:
```c
// powerblocks/core/bluetooth/l2cap.c

#define L2CAP_TASK_STACK_SIZE  8192

#define L2CAP_TASK_PRIORITY    (configMAX_PRIORITIES / 4 * 3)

static const char* TAG = "L2CAP";
```

Using underscores at the beginning of names should be avoided for clarity.
```c
// Do not do this:
#define ___MY_DEFINITION__ 1

static void _cool_static_function(int ibxpr) {
	// ...
}
```
### Formatting
 * 4 spaces (not tabs.)
 * Opening braces on the same line.
 * Keep lines under ~100 characters.
 * At least 1 blank line between functions.


# Testing
Please make sure to provide testing of your contribution.

Emulator testing is good during development, but please work to provide testing on hardware. The Wii's hardware usually behaves differently from emulator, so hardware testing is essential.

It is also fine to request that I or another contributor test specific features. I myself do not own many of the peripherals and expansions for the Wii. It is fine if these features are hardened with time as they are used as well.

Always be careful about making sure to not brick the Wii during testing. It is generally a good idea to have a solid Wii homebrew setup to avoid this.

Refer to [Wii Hacks Guide](https://wii.hacks.guide/) to help set up a Wii for homebrew development.
 <!-- Doxygen navigation -->
\subpage code_of_conduct "Code of Conduct"