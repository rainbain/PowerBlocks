\page windows Windows Installation

PowerBlocks provides downloadable releases for Windows.

---
\note
These instructions may be incomplete.  Please report any missing steps or issues to the [Issue Tracker](https://github.com/rainbain/PowerBlocks/issues)

# Prerequisites
PowerBlock's runs inside of a "Linux like" environment. In order to provide this MSYS2 must be installed from [here](https://www.msys2.org/).

From the MSYS2 shell, install the required build tools.
```bash
pacman -S cmake ninja wget unzip
```

---
# Download the SDK
From the MSYS2 shell, download and unzip the latest version of the SDK for Windows from the [PowerBlocks Releases Page](https://github.com/rainbain/PowerBlocks/releases).

```bash
cd ~
wget https://github.com/rainbain/PowerBlocks/releases/download/<VERSION>/PowerBlocks-windows.zip
unzip PowerBlocks-windows.zip 
rm PowerBlocks-windows.zip
```
---
# Start a Project
We will create a project folder and copy in the `HelloWorld` example.

```bash
mkdir MyProject
cd MyProject
cp -r ~/PowerBlocks-windows/examples/HelloWorld/. .
```

## Build the Project
To build it we will run PowerBlocks' export script to add the compilers and toolchain to the path and build the project.

```bash
mkdir build
cd build
. ~/PowerBlocks-windows/export.sh
cmake ..
ninja
```

You should now see a `HelloWorld.elf` inside the `build/` folder of your project.