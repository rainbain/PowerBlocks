cmake_minimum_required(VERSION 3.16)
project(HelloWorld C)

find_package(PowerBlocks REQUIRED)
include(RustUtils)

add_executable(HelloWorld.elf main.c)

add_rust_library(rust_lib ${CMAKE_SOURCE_DIR}/rust-lib)

target_link_libraries(HelloWorld.elf PUBLIC PowerBlocks::Common PowerBlocks::Core rust_lib)