cmake_minimum_required(VERSION 3.16)
project(FileSystem C)

find_package(PowerBlocks REQUIRED)

add_executable(FileSystem.elf main.c)

target_link_libraries(FileSystem.elf PUBLIC PowerBlocks::Common PowerBlocks::Core PowerBlocks::FileSystem)