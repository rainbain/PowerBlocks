add_library(
    PowerBlocksCore

    STATIC

    system/start.s
    system/exceptions_asm.s
    system/system_asm.s
    system/libcio.c
    system/system.c
    system/exceptions.c

    graphics/video.c
    graphics/framebuffer.c

    utils/fonts.c
    utils/console.c
    utils/math/arith64.c
)

add_library(PowerBlocks::Core ALIAS PowerBlocksCore)

target_link_libraries(PowerBlocksCore PRIVATE PowerBlocks::Common)

target_include_directories(PowerBlocksCore PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})