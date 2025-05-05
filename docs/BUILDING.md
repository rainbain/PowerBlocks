# Building the SDK
Building the SDK is quite a process as it currently stands. It mainly involves bringing together all the tools needed to operate it.

## Building Toolchain
First you need to setup the clang toolchain. Creating both the linux and MSYS2 build works on linux. I have trouble getting llvm to build in MSYS2.

Export build environment.
```
. ./export_build.sh
```

Navigate to a working directory for cloning LLVM.

Clone LLVM and setup build folder.
```
git clone https://github.com/llvm/llvm-project.git
cd llvm-project
mkdir build
cd build
```

Configure LLVM. This will use the MinGW if windows is your target. Delete
the line with `CMAKE_TOOLCHAIN_FILE` if you want to use it on linux.
```
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

Build and install it.
```
ninja
ninja install-clang install-lld install-llvm-ar install-llvm-ranlib install-llvm-objcopy install-llvm-strip install-llvm-objdump install-clang-resource-headers
```

If you wish to export to windows, you will need to fix the symbolic links.
```
cd ${SDK_HOME}/toolchain/
python3 fix_symlink.py
```

If exporting to windows, its often DLL files will be missing.
```
cp /usr/lib/gcc/x86_64-w64-mingw32/14-win32/libgcc_s_seh-1.dll ${SDK_HOME}/toolchain/bin/
cp /usr/lib/gcc/x86_64-w64-mingw32/14-win32/libstdc++-6.dll ${SDK_HOME}/toolchain/bin/
```

## Building Picolibc
At this point transition to the system you intend to use the SDK on.

Export SDK.
```
. ./export.sh
```

Navigate to a working folder, clone picolibc and setup build folder.
```
git clone https://github.com/picolibc/picolibc.git
cd picolibc
mkdir build
cd build
```

Copy in patched setjmp.S file.
```
cp ${SDK_HOME}/picolibc/setjmp.S ../newlib/libc/machine/powerpc/setjmp.S
```

Setup environment variables.
```
export CMAKE_GENERATOR="Unix Makefiles"
```

Configure it
```
cmake .. -DCMAKE_INSTALL_PREFIX=${SDK_HOME}/picolibc -DEXTRA_C_FLAGS="-I${SDK_HOME}/picolibc/"
```

Make and install
```
make -j$(nproc)
make install
```