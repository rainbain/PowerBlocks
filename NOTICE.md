# Third-Party Software Notices

This project includes third-party libraries and files that are licensed under different terms. Please see below for detailed attribution and license information for each.

## Project License

This project is licensed under the **MIT License**. See the `LICENSE` file for more details.

---

## Third-Party Components

### 1. **FreeRTOS Kernel**

- **Source**: https://www.freertos.org
- **License**: MIT License
- **Path**: `third_party/freertos/`
- **Notes**: The FreeRTOS kernel is provided under the MIT License.

---

### 2. **GCC - PowerPC Assembly Header (`ppc-asm.h`)**

- **Source**: Part of GCC, provided along side Picolibc
- **License**: GPLv3+ with GCC Runtime Library Exception v3.1
- **Path**: `third_party/picolibc/ppc-asm.h`
- **Notes**: This file is licensed under the GPLv3+ with the GCC Runtime Library Exception. Redistribution is permitted when used as part of a runtime library.

---

### 3. **picolibc - Setjmp (`setjmp.S`)**

- **Source**: Picolibc
- **License**: 4-Clause BSD (Original BSD License)
- **Path**: `third_party/picolibc/setjmp.S`
- **Notes**: This file is licensed under the Original BSD License. Redistribution and use in source and binary forms are permitted with attribution. It has also been patched to fix branch instructions for PowerPC 750.

---

### 4. **picolibc**

- **Source**: Picolibc
- **License**: BSD-like (various)
- **Path**: `third_party/picolibc/include/`
- **Notes**: These header files are part of Picolibc, which uses various BSD-style licenses. For full license details, see `picolibc/COPYING.picolibc`. This is included in releases when picolibc is installed.