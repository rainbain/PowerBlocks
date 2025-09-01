cmake_minimum_required(VERSION 3.16)
project(Teapot C)

find_package(PowerBlocks REQUIRED)

add_executable(Teapot.elf main.c utah_teapot.c utah_teapot_model.c utah_teapot_texture.c)

target_link_libraries(Teapot.elf PUBLIC PowerBlocks::Common PowerBlocks::Core)