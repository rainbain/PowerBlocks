cmake_minimum_required(VERSION 3.16)
project(wiimote C)

find_package(PowerBlocks REQUIRED)

add_executable(wiimote.elf main.c)

target_link_libraries(wiimote.elf PUBLIC PowerBlocks::Common PowerBlocks::Core PowerBlocks::Input)