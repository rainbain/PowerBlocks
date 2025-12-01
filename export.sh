#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# SDK
export SDK_HOME="$SCRIPT_DIR"
export EXTRA_C_FLAGS=""

# Toolchain
export TOOLCHAIN="$SCRIPT_DIR/toolchain"
export PATH="$TOOLCHAIN/bin:$PATH"

# Build Options
export CMAKE_TOOLCHAIN_FILE="$SCRIPT_DIR/toolchain.cmake"
export CMAKE_GENERATOR="Ninja"
export CMAKE_BUILD_TYPE=Release

# CMake Find Package
export CMAKE_PREFIX_PATH="$SCRIPT_DIR:$SCRIPT_DIR/tools/dspasm"