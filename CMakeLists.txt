cmake_minimum_required(VERSION 3.16)

project(DarkTube C CXX)

# devkitPro Switch CMake setup
if(NOT DEFINED ENV{DEVKITPRO})
    message(FATAL_ERROR "Please set DEVKITPRO in your environment")
endif()

set(DEVKITPRO $ENV{DEVKITPRO})
list(APPEND CMAKE_MODULE_PATH "${DEVKITPRO}/cmake")

include(Switch)

set(OUTPUT_NAME "DarkTube")

# Sources
file(GLOB_RECURSE SOURCES "source/*.c" "source/*.cpp" "cjson/*.c")
include_directories(source include cjson)

# Portlibs
set(PORTLIBS "${DEVKITPRO}/portlibs/switch")
include_directories("${PORTLIBS}/include")
include_directories("${PORTLIBS}/include/jpeg")
include_directories("${PORTLIBS}/include/libpng16")
link_directories("${PORTLIBS}/lib")

# Compiler flags
add_definitions(-D__SWITCH__)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

# Main executable
add_executable(${OUTPUT_NAME} ${SOURCES})

# SEMUA library dalam satu group besar untuk menangani circular dependencies
target_link_libraries(${OUTPUT_NAME}
    -Wl,--start-group
    # System compression (URUTAN PENTING - letakkan di awal group)
    lzma
    zstd
    z
    bz2
    
    # Image libraries
    jpeg
    png16
    
    # Archive library (membutuhkan semua compression libs)
    archive
    
    # Core media libraries (FFmpeg)
    mpv
    placebo
    avformat
    avfilter
    postproc
    avcodec
    dav1d
    swresample
    swscale
    avutil
    
    # Subtitle and font libraries
    ass
    freetype
    fribidi
    harfbuzz
    expat
    
    # Audio codecs
    theora
    vorbis
    vorbisenc
    ogg
    
    # Network and crypto
    curl
    mbedtls
    mbedx509
    mbedcrypto
    
    # Graphics and system libraries
    SDL2_ttf
    SDL2_image
    SDL2
    EGL
    GLESv2
    glapi
    drm_nouveau
    webp
    
    # Switch system library
    nx
    stdc++
    m
    -Wl,--end-group
)

# ── NRO Generation ────────────────────────────────────────────────────────────

set(APP_TITLE   "DarkTube")
set(APP_AUTHOR  "Antigravity and User")
set(APP_VERSION "2.0.0")

set(NRO_OUTPUT "${CMAKE_BINARY_DIR}/${OUTPUT_NAME}.nro")
set(NACP_FILE  "${CMAKE_BINARY_DIR}/${OUTPUT_NAME}.nacp")

# 1. Buat NACP
add_custom_command(
    OUTPUT ${NACP_FILE}
    COMMAND ${NX_NACPTOOL_EXE}
        --create "${APP_TITLE}" "${APP_AUTHOR}" "${APP_VERSION}"
        ${NACP_FILE}
    DEPENDS ${OUTPUT_NAME}
    COMMENT "Generating NACP metadata..."
)

# 2. Pilih icon
if(EXISTS "${CMAKE_SOURCE_DIR}/assets/icon.jpg")
    set(ICON_ARG "--icon=${CMAKE_SOURCE_DIR}/assets/icon.jpg")
elseif(DEFINED NX_DEFAULT_ICON AND EXISTS "${NX_DEFAULT_ICON}")
    set(ICON_ARG "--icon=${NX_DEFAULT_ICON}")
else()
    set(ICON_ARG "")
endif()

# 3. ELF -> NRO
add_custom_command(
    OUTPUT ${NRO_OUTPUT}
    COMMAND ${NX_ELF2NRO_EXE}
        $<TARGET_FILE:${OUTPUT_NAME}>
        ${NRO_OUTPUT}
        --nacp=${NACP_FILE}
        ${ICON_ARG}
    DEPENDS ${OUTPUT_NAME} ${NACP_FILE}
    COMMENT "Converting ELF to NRO..."
)

# 4. Target build utama
add_custom_target(${OUTPUT_NAME}_nro ALL
    DEPENDS ${NRO_OUTPUT}
)