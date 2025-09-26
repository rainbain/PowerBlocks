include(ExternalProject)

function(add_rust_library NAME CRATE_DIR)
    set(RUST_TARGET_DIR ${CMAKE_BINARY_DIR}/rust-target)
    set(RUST_JSON ${CRATE_DIR}/powerpc750.json)
    set(RUST_LIB_PATH ${RUST_TARGET_DIR}/powerpc750/release/lib${NAME}.a)

    ExternalProject_Add(
        ${NAME}_ext
        DOWNLOAD_COMMAND ""
        CONFIGURE_COMMAND ""
        BUILD_COMMAND cargo +nightly build --release
                      --target ${RUST_JSON}
                      --target-dir ${RUST_TARGET_DIR}
                      -Z build-std=core
        BINARY_DIR ${CRATE_DIR}
        INSTALL_COMMAND ""
        BUILD_BYPRODUCTS ${RUST_LIB_PATH}
        LOG_BUILD 1
        LOG_OUTPUT_ON_FAILURE 1
    )

    add_library(${NAME} STATIC IMPORTED)
    set_target_properties(${NAME} PROPERTIES IMPORTED_LOCATION ${RUST_LIB_PATH})

    add_dependencies(${NAME} ${NAME}_ext)
endfunction()
