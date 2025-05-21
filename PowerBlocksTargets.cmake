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

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/powerblocks" "${CMAKE_BINARY_DIR}/powerblocks")