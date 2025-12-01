add_library(PowerBlocksCommon INTERFACE)
target_include_directories(PowerBlocksCommon INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}
)

# Add picolibc to PowerBlocks::Common
target_include_directories(PowerBlocksCommon INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/picolibc/include
)
target_link_libraries(
  PowerBlocksCommon INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/picolibc/lib/libc.a
)

add_library(PowerBlocks::Common ALIAS PowerBlocksCommon)

set(FREERTOS_PATH ${CMAKE_CURRENT_LIST_DIR}/third_party/freertos)
set(FATFS_PATH ${CMAKE_CURRENT_LIST_DIR}/third_party/fatfs)
set(QRCODEGEN_PATH ${CMAKE_CURRENT_LIST_DIR}/third_party/qrcodegen)

# Include DSP Microcode Tools
add_subdirectory(
    ${CMAKE_CURRENT_LIST_DIR}/tools/dspasm
    ${CMAKE_CURRENT_BINARY_DIR}/tools/dspasm_build # Here to make cmake not complain about out of
)

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/powerblocks" "${CMAKE_BINARY_DIR}/powerblocks")