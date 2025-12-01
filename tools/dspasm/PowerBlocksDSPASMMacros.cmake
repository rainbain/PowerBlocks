find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(DSPASM_CLI_PATH "${CMAKE_CURRENT_LIST_DIR}/dspasm_cli.py")

macro(dsp_assemble target_name)
    set(SOURCE_FILES ${ARGN})

    if(NOT SOURCE_FILES)
        message(FATAL_ERROR "No source files provided to dsp_assemble for target ${target_name}")
    endif()

    set(DSP_GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/dsp_microcode)
    file(MAKE_DIRECTORY ${DSP_GENERATED_DIR})
    set(ABS_OUTPUT ${DSP_GENERATED_DIR}/${target_name}.bin)

    # Only rebuild if ABS_OUTPUT is missing or outdated
    add_custom_command(
        OUTPUT ${ABS_OUTPUT}
        COMMAND ${Python3_EXECUTABLE} ${DSPASM_CLI_PATH} ${SOURCE_FILES} -o ${ABS_OUTPUT}
        DEPENDS ${SOURCE_FILES} ${DSPASM_CLI_PATH}
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        COMMENT "Assembling DSP microcode: ${SOURCE_FILES} -> ${ABS_OUTPUT}"
        VERBATIM
    )

    # Target depends on the generated file
    add_custom_target(${target_name} DEPENDS ${ABS_OUTPUT})

    # Export path for convenience
    set(${target_name}_BINARY ${ABS_OUTPUT} CACHE INTERNAL "DSP microcode binary for ${target_name}")
endmacro()
