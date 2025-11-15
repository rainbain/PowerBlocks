add_library(
    PowerBlocksFileSystem

    STATIC

    sd.c
    fs_syscall.c

    fatfs_port/diskio.c

    ${FATFS_PATH}/ff.c
    ${FATFS_PATH}/ffsystem.c
    ${FATFS_PATH}/ffunicode.c
)

add_library(PowerBlocks::FileSystem ALIAS PowerBlocksFileSystem)

target_link_libraries(PowerBlocksFileSystem PRIVATE PowerBlocks::Common)

target_include_directories(PowerBlocksFileSystem PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_include_directories(PowerBlocksCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/fatfs_port)
target_include_directories(PowerBlocksCore PUBLIC ${FATFS_PATH})

target_link_libraries(PowerBlocksFileSystem
    PUBLIC
        PowerBlocks::Core
)