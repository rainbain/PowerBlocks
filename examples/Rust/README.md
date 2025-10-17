# Rust
Example using Rust Coding Language inside of PowerBlocks.

You will need to have Rust installed on the system, preferably nightly,
so that it can compile libcore automatically. Otherwise this example
will not work.

You will need the rust source too so that it can build libcore.
```
rustup component add rust-src
```

To build it first export the sdk.
```
. ./export.sh
```

If you are on Msys2, and have Rust installed on Windows,
you can access it from Msys2 by adding it to the path temporarily.
```
export PATH=$PATH:`cygpath $USERPROFILE/.cargo/bin`
```

Build the example:
```
mkdir build
cd build
cmake ..
ninja
```

From here a .elf file is provided. You can convert this to .dol with an external tool or use the ELF directly.

It is recommended to launch directly through Homebrew Channel so that the correct system environment is set up.