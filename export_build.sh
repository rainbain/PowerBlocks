#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# SDK
export SDK_HOME="$SCRIPT_DIR"
export TOOLCHAIN="$SCRIPT_DIR/toolchain"