# Skip compiler test
set(CMAKE_C_COMPILER_WORKS 1)

# System
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR powerpc)

# Compiler
set(triple ppc32-unknown-elf)
set(CMAKE_C_COMPILER clang)
set(CMAKE_C_COMPILER_TARGET ${triple})
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_COMPILER_TARGET ${triple})
set(CMAKE_ASM_COMPILER clang)
set(CMAKE_ASM_COMPILER_TARGET ${triple})

get_filename_component(TOOLCHAIN_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
set(LINKER_SCRIPT "${TOOLCHAIN_DIR}/linker.ld")

# This was really annoying. Let me know if there is a better fix.
# Clang was invoking GCC for phrasing command line arguments for the linker.
# Even if I set use lld in build and said -fuse-ld=lld. The problem with
# doing this was in msys2 it would try to add its dll search prefix.
# This would then be pasted to ld.lld, it would not understand that, and error out.
set(CMAKE_C_LINK_EXECUTABLE "ld.lld -T${LINKER_SCRIPT} <CMAKE_C_LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")

# Compiler flags
set(CMAKE_C_FLAGS "-mcpu=750 -ffreestanding -nostdlib -I${CMAKE_CURRENT_LIST_DIR} ${EXTRA_C_FLAGS}")
set(CMAKE_ASM_FLAGS "-I${CMAKE_CURRENT_LIST_DIR} ${EXTRA_C_FLAGS} -Qunused-arguments")