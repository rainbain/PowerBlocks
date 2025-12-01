add_library(
    PowerBlocksDebugger

    STATIC

    debugger.c
    ${QRCODEGEN_PATH}/qrcodegen.c
)

add_library(PowerBlocks::Debugger ALIAS PowerBlocksDebugger)

target_link_libraries(PowerBlocksDebugger PRIVATE PowerBlocks::Common)

target_include_directories(PowerBlocksDebugger PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_include_directories(PowerBlocksCore PUBLIC ${QRCODEGEN_PATH})

target_link_libraries(PowerBlocksDebugger
    PUBLIC
        PowerBlocks::Core
)