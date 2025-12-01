"""
DSP Microcode Assembler Client Interface

Provides a standard command line interface
on-top of the assembler.

Author: Samuel Fitzsimons (rainbain)
File: dspasm_cli.py
Date: 2025
"""
#!/usr/bin/env python3

import argparse
import sys
import os
from pathlib import Path

from dspasm.preprocessor import Preprocessor
from dspasm.utils import dump_tokens_to_file

def main():
    parser = argparse.ArgumentParser(description="PowerBlocks SDK Audio DSP Microcode Assembler")
    parser.add_argument("input", type=Path, help="Input .s assembly source file")
    parser.add_argument("-o", "--output", type=Path, help="Output binary file", default="out.bin")
    parser.add_argument("-t", "--tokens", type=Path, help="Dump tokens after preprocessor to file for debugging.")
    parser.add_argument("-bt", "--backtrace", help="Enable python backtrace on errors.", action="store_true")
    
    args = parser.parse_args()

    input_source = args.input

    try:
        preprocessor = Preprocessor()

        # Recursive include
        tokens = preprocessor.recursive_include(input_source)

        # Take on all the macros
        tokens = preprocessor.gather(tokens)

        # Do not allow circular definitions
        preprocessor.check_circular_definitions()

        # Evaluate
        tokens = preprocessor.run(tokens)
    except Exception as e:
        # If backtrace, send this off to the top
        if args.backtrace:
            raise e

        print(f"{os.path.basename(input_source)}: Preprocessor Failed")
        print(f"\t{e}")
        sys.exit(1)

    # Dump them if asked
    if args.tokens:
        dump_tokens_to_file(tokens, args.tokens)


if __name__ == "__main__":
    main()