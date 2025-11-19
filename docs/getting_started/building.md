\page building Build From Source
This page covers how to build PowerBlocks SDK from source on **Linux** and **Windows**.

These steps must be followed on a **Linux** host machine.

Building PowerBlocks has the following steps.
1. Download & Export Environment
2. Build LLVM & Clang
3. Build Picolibc

---
# Download & Export Environment
Clone the source code to your workspace.
```sh
mkdir ~/Workspace
cd ~/Workspace
git clone https://github.com/rainbain/PowerBlocks.git
```

And export the build environment to the path.
```sh
. ./PowerBlocks/export_build.sh
```

---
# Build LLVM & Clang
Clone the latest version of LLVM and set up build folder.
```
git clone https://github.com/llvm/llvm-project.git
cd llvm-project
mkdir build
cd build
```

Configure LLVM. You can configure it to generate binaries for Windows or Linux.

## Linux
```sh
cmake -G Ninja ../llvm \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=${SDK_HOME}/toolchain \
  -DLLVM_TARGETS_TO_BUILD="PowerPC" \
  -DLLVM_ENABLE_PROJECTS="clang;lld" \
  -DLLVM_ENABLE_LLD=ON \
  -DLLVM_ENABLE_ASSERTIONS=OFF \
  -DLLVM_INCLUDE_EXAMPLES=OFF \
  -DLLVM_INCLUDE_TESTS=OFF \
  -DLLVM_INCLUDE_DOCS=OFF \
  -DCLANG_INCLUDE_TESTS=OFF \
  -DCLANG_INCLUDE_DOCS=OFF \
  -DLLVM_ENABLE_BACKTRACES=OFF
```

## Windows
```sh
cmake -G Ninja ../llvm \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=${SDK_HOME}/toolchain/mingw-toolchain.cmake \
  -DCMAKE_INSTALL_PREFIX=${SDK_HOME}/toolchain \
  -DLLVM_TARGETS_TO_BUILD="PowerPC" \
  -DLLVM_ENABLE_PROJECTS="clang;lld" \
  -DLLVM_ENABLE_LLD=ON \
  -DLLVM_ENABLE_ASSERTIONS=OFF \
  -DLLVM_INCLUDE_EXAMPLES=OFF \
  -DLLVM_INCLUDE_TESTS=OFF \
  -DLLVM_INCLUDE_DOCS=OFF \
  -DCLANG_INCLUDE_TESTS=OFF \
  -DCLANG_INCLUDE_DOCS=OFF \
  -DLLVM_ENABLE_BACKTRACES=OFF
```

## Now Build & Install
```sh
ninja
ninja install-clang install-lld install-llvm-ar install-llvm-ranlib install-llvm-objcopy install-llvm-strip install-llvm-objdump install-clang-resource-headers
```

##  Windows Setup
### Fix Symbolic Links
In order to run on Windows, symbolic links must be fixed.
```sh
cd ${SDK_HOME}/toolchain/
python3 fix_symlink.py
```

### Copy DLL Files
Windows will need the MinGW runtime DLL files. The exact path may depend on your MinGW version.
```sh
cp /usr/lib/gcc/x86_64-w64-mingw32/14-win32/libgcc_s_seh-1.dll ${SDK_HOME}/toolchain/bin/
cp /usr/lib/gcc/x86_64-w64-mingw32/14-win32/libstdc++-6.dll ${SDK_HOME}/toolchain/bin/
```

---
# Building Picolibc
Now we want to build executables for the `Wii`, not the SDK target host machine. So we will use the compilers we just made.
```sh
cd ~/Workspace
. ./PowerBlocks/export.sh
```

Clone Picolibc and setup build folder.
```sh
git clone https://github.com/picolibc/picolibc.git
cd picolibc
mkdir build
cd build
```

Copy the patched `setjmp.S` file for PowerPC 750.
```sh
cp ${SDK_HOME}/picolibc/setjmp.S ../newlib/libc/machine/powerpc/setjmp.S
```

Enforce using **Makefiles** for picolibc's build system. The build commands have been known to be too long for ninja.
```sh
export CMAKE_GENERATOR="Unix Makefiles"
```

Configure it:
```sh
cmake .. -DCMAKE_INSTALL_PREFIX=${SDK_HOME}/picolibc -DEXTRA_C_FLAGS="-I${SDK_HOME}/picolibc/"
```

Build and Install:
```sh
make -j$(nproc)
make install
```

Lastly, copy over the license file for picolibc:
```sh
cp ../COPYING.picolibc ${SDK_HOME}/picolibc/COPYING.picolibc
```

---
# Conclusion
The PowerBlocks SDK that we cloned earlier should now have the compilers and c runtime installed.

It is ready to be used like the builds on the [PowerBlocks Releases Page](https://github.com/rainbain/PowerBlocks/releases).