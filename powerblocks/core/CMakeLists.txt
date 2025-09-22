add_library(
    PowerBlocksCore

    STATIC

    system/start.s
    system/exceptions_asm.s
    system/system_asm.s
    system/libcio.c
    system/system.c
    system/exceptions.c
    system/syscall.c
    system/ipc.c
    system/ios.c
    system/ios_settings.c
    system/gpio.c

    graphics/video.c
    graphics/framebuffer.c
    graphics/gx.c

    utils/fonts.c
    utils/console.c
    utils/log.c
    utils/crash_handler.c
    utils/math/arith64.c
    utils/math/floatdidf.c
    utils/math/vec3.c
    utils/math/matrix4.c
    utils/math/matrix34.c
    utils/math/matrix3.c

    bluetooth/hci.c
    bluetooth/l2cap.c
    bluetooth/bltools.c

    freertos_port/port.c
    ${FREERTOS_PATH}/tasks.c
    ${FREERTOS_PATH}/timers.c
    ${FREERTOS_PATH}/stream_buffer.c
    ${FREERTOS_PATH}/queue.c
    ${FREERTOS_PATH}/list.c
    ${FREERTOS_PATH}/portable/MemMang/heap_3.c
)

add_library(PowerBlocks::Core ALIAS PowerBlocksCore)

target_link_libraries(PowerBlocksCore PRIVATE PowerBlocks::Common)

target_include_directories(PowerBlocksCore PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_include_directories(PowerBlocksCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/freertos_port)

target_include_directories(PowerBlocksCore PUBLIC ${FREERTOS_PATH}/include)