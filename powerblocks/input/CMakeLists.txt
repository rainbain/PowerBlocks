add_library(
    PowerBlocksInput

    STATIC

    wiimote/wiimote.c
    wiimote/wiimote_extension.c
    wiimote/wiimote_hid.c
    wiimote/wiimote_sys.c
)

add_library(PowerBlocks::Input ALIAS PowerBlocksInput)

target_link_libraries(PowerBlocksInput PRIVATE PowerBlocks::Common)

target_include_directories(PowerBlocksInput PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(PowerBlocksInput
    PUBLIC
        PowerBlocks::Core
)