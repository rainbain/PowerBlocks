\page docker Docker Installation

PowerBlocks provides ready to go Docker packages allowing easy use across all platforms.

---
\note
These instructions have not been tested on macOS. 
If you try them and they work, please confirm by submitting a comment or issue in the [PowerBlocks Issue Tracker](https://github.com/rainbain/PowerBlocks/issues). 
Once macOS compatibility is verified, this note will be removed.
# Prerequisites

To begin, you will need an installation of Docker installed. The version of Docker depends on the host OS.


## Linux

For Linux you can install Docker Engine directly from [here](https://docs.docker.com/engine/).


## Windows & MacOS

For these systems, Docker Desktop is required and can be setup from [here](https://docs.docker.com/desktop/).

---

# Start A Project

## Create Project Folder

To begin, a PowerBlocks project using a Docker container, first locate the [latest PowerBlock's package](https://github.com/rainbain/PowerBlocks/pkgs/container/powerblocks).

Then, in a terminal, create a workspace. This can be the name of your project.

### Example

```
mkdir MyProject
cd MyProject
```



## Run Docker Container

Now run the Docker container from inside your project folder. We will mount the project folder inside the container as `/workspace`.

Make sure to replace `<VERSION>` with your version of PowerBlocks.


### Linux & macOS

```

docker run --rm -it -v "$PWD:/workspace" ghcr.io/rainbain/powerblocks:slim-<VERSION>
```


### Windows PowerShell

```
docker run --rm -it -v "${PWD}:/workspace" ghcr.io/rainbain/powerblocks:slim-<VERSION>
```


## Create Project

To create the project, copy an example from the SDK into your current project folder.


### Example

```
cp -r powerblocks/examples/HelloWorld/. /workspace/
```

At this point, a PowerBlock's project is created within your project folder.

## Build the Project

Whole still operating inside the Docker container, you can now configure and build the project.
### Build Commands

```
cd /workspace
mkdir build
cd build
cmake ..
ninja
```

You should now see a `HelloWorld.elf` inside the `build/` folder of your project.
