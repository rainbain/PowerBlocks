\page linux Linux Installation

PowerBlocks provides downloadable releases for Linux.

---
\note
These instructions may be incomplete.  Please report any missing steps or issues to the [Issue Tracker](https://github.com/rainbain/PowerBlocks/issues)

# Prerequisites
To begin, install the required build tools using your package manager of choice.

```bash
sudo apt install cmake ninja-build wget unzip
```

---
# Download the SDK
Then download and unzip the latest version of the SDK for Linux from the [PowerBlocks Releases Page](https://github.com/rainbain/PowerBlocks/releases).

Replace `<VERSION>` with the latest version.
```bash
cd ~
wget https://github.com/rainbain/PowerBlocks/releases/download/<VERSION>/PowerBlocks-linux.zip
unzip PowerBlocks-linux.zip 
rm PowerBlocks-linux.zip
```

---
# Start a Project
We will create a project folder and copy in the `HelloWorld` example.

```bash
mkdir MyProject
cd MyProject
cp -r ~/PowerBlocks-linux/examples/HelloWorld/. .
```

## Build the Project
To build it we will run PowerBlocks' export script to add the compilers and toolchain to the path and build the project.

```bash
mkdir build
cd build
. ~/PowerBlocks-linux/export.sh
cmake ..
ninja
```

You should now see a `HelloWorld.elf` inside the `build/` folder of your project.