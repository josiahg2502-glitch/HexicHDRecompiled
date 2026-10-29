# hexic - ReXGlue Recompiled Project
#
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(hexic LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(generated/rexglue.cmake)

# Sources
set(HEXIC_SOURCES
    src/main.cpp
    src/crash_log.cpp
    src/script_input.cpp
    src/stfs_extract.cpp
)

if(WIN32)
    add_executable(hexic WIN32 ${HEXIC_SOURCES})
else()
    add_executable(hexic ${HEXIC_SOURCES})
endif()

rexglue_setup_target(hexic GPU_PLUGINS xenos)

# Precompiled shader storage (built with tools/warm_shader_cache.ps1) ships next to the exe;
# the app seeds the user's cache from it at startup.
add_custom_command(TARGET hexic POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/shader_cache $<TARGET_FILE_DIR:hexic>/shader_cache
    VERBATIM)

# A PDB with public symbols, so native stacks resolve to the recompiled sub_XXXXXXXX names.
if(WIN32)
    target_link_options(hexic PRIVATE "LINKER:/DEBUG")
endif()
