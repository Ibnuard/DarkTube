cmake_minimum_required(VERSION 3.16)

project(DarkTube C CXX)

# devkitPro Switch CMake setup
if(NOT DEFINED ENV{DEVKITPRO})
    message(FATAL_ERROR "Please set DEVKITPRO in your environment")
endif()

set(DEVKITPRO $ENV{DEVKITPRO})
list(APPEND CMAKE_MODULE_PATH "${DEVKITPRO}/cmake")

# Target NRO output
set(OUTPUT_NAME "DarkTube")

# Include libnx modules
include(Switch)

# Sources
file(GLOB_RECURSE SOURCES "source/*.c" "source/*.cpp" "cjson/*.c")
include_directories(source include cjson)

# Compiler flags for Switch
add_definitions(-D__SWITCH__)

# Use modern C++
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 11)

# Main Executable
add_executable(${OUTPUT_NAME}.elf ${SOURCES})

# Libraries
# Base libs provided by devkitPro
target_link_libraries(${OUTPUT_NAME}.elf
    nx
    m
    z
    curl
    mbedtls
    mbedx509
    mbedcrypto
)

# Portlibs path
set(PORTLIBS "${DEVKITPRO}/portlibs/switch")
target_include_directories(${OUTPUT_NAME}.elf PRIVATE "${PORTLIBS}/include")
target_link_directories(${OUTPUT_NAME}.elf PRIVATE "${PORTLIBS}/lib")

# Add multimedia libs for libmpv
target_link_libraries(${OUTPUT_NAME}.elf
    mpv
    ffmpeg
    ass
    freetype
    fribidi
    fontconfig
    expat
    harfbuzz
    bluray
    theora
    vorbis
    vorbisenc
    ogg
    opus
    vpx
    x264
    x265
    webp
    bz2
    lzma
    iconv
    xml2
)

# Build NRO from ELF
# Arguments: target NAME AUTHOR VERSION ICON
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/data/icon.jpg")
    add_nro_target(${OUTPUT_NAME}.elf
        NAME "DarkTube"
        AUTHOR "Antigravity & User"
        VERSION "2.0.0"
        ICON "${CMAKE_CURRENT_SOURCE_DIR}/data/icon.jpg"
    )
else()
    add_nro_target(${OUTPUT_NAME}.elf
        NAME "DarkTube"
        AUTHOR "Antigravity & User"
        VERSION "2.0.0"
    )
endif()
