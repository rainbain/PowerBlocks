cmake_minimum_required(VERSION 3.16)
project(HelloWorld C)

find_package(PowerBlocks REQUIRED)

add_executable(HelloWorld.elf main.c)

target_link_libraries(HelloWorld.elf PUBLIC PowerBlocks::Common PowerBlocks::Core)