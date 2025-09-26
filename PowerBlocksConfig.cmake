enable_language(ASM)

include("${CMAKE_CURRENT_LIST_DIR}/PowerBlocksTargets.cmake")

# Add cmake module folder
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
