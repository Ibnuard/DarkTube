cmake_minimum_required(VERSION 3.13)

set(BOREALIS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/library/borealis)
set(BOREALIS_LIBRARY ${BOREALIS_DIR}/library)

# Include Platform common options
include(${BOREALIS_LIBRARY}/cmake/commonOption.cmake)

# Dependencies
option(USE_SHARED_LIB "Whether to use shared libs provided by system" OFF)
option(USE_SYSTEM_SDL2 "" ${USE_SHARED_LIB})
option(USE_SYSTEM_GLFW "" OFF)

# mpv related
option(MPV_SW_RENDER "Using CPU to draw videos" OFF)
if (MPV_SW_RENDER)
    list(APPEND APP_PLATFORM_OPTION -DMPV_SW_RENDER)
endif ()

option(MPV_NO_FB "Using system provided framebuffer" OFF)
if (MPV_NO_FB)
    list(APPEND APP_PLATFORM_OPTION -DMPV_NO_FB)
endif()

# toolchain
include(${BOREALIS_LIBRARY}/cmake/toolchain.cmake)

# Project Info
project(DarkTube)
set(VERSION_MAJOR "1")
set(VERSION_MINOR "0")
set(VERSION_REVISION "0")
if (NOT VERSION_BUILD)
    set(VERSION_BUILD "0")
endif ()
set(PROJECT_AUTHOR "Dev")
set(PACKAGE_NAME com.dev.darktube)
set(PROJECT_ICON ${CMAKE_CURRENT_SOURCE_DIR}/resources/icon/icon.jpg)
set(PROJECT_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/resources)
set(APP_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_REVISION}")

find_package(Threads REQUIRED)
list(APPEND APP_PLATFORM_LIB ${CMAKE_THREAD_LIBS_INIT})

# Set specific Switch target
set(PLATFORM_SWITCH ON)

# Cross compile (Switch)
find_package(PkgConfig REQUIRED)
pkg_search_module(MPV REQUIRED mpv)
message(STATUS "Found libmpv: ${MPV_VERSION} ${MPV_INCLUDE_DIRS} ${MPV_STATIC_LIBRARIES}")
list(APPEND APP_PLATFORM_INCLUDE ${MPV_INCLUDE_DIRS})
list(APPEND APP_PLATFORM_LIB ${MPV_STATIC_LIBRARIES})
link_directories(${MPV_LIBRARY_DIRS})

# CURL
pkg_search_module(CURL REQUIRED libcurl)
list(APPEND APP_PLATFORM_INCLUDE ${CURL_INCLUDE_DIRS})
list(APPEND APP_PLATFORM_LIB ${CURL_STATIC_LIBRARIES})

list(APPEND APP_PLATFORM_OPTION
   -DBUILD_PACKAGE_NAME=${PACKAGE_NAME}
   -DBUILD_VERSION_MAJOR=${VERSION_MAJOR}
   -DBUILD_VERSION_MINOR=${VERSION_MINOR}
   -DBUILD_VERSION_REVISION=${VERSION_REVISION}
)

if (USE_LIBROMFS)
    add_libromfs(${PROJECT_NAME} ${PROJECT_RESOURCES})
endif ()

# set resources dir
if (CUSTOM_RESOURCES_DIR)
    set(BRLS_RESOURCES_DIR ${CUSTOM_RESOURCES_DIR})
else ()
    set(BRLS_RESOURCES_DIR ".")
endif ()

# source files
file(GLOB_RECURSE MAIN_SRC darktube/source/*.cpp)
list(APPEND MAIN_SRC ${BOREALIS_LIBRARY}/lib/platforms/switch/switch_wrapper.c)

# build borealis
add_subdirectory(library/borealis/library)

# Main Target
program_target(${PROJECT_NAME} "${MAIN_SRC}")
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)

# building release file
if (USE_DEKO3D)
    gen_dksh("${PROJECT_RESOURCES}/shaders")
endif ()
add_custom_target(${PROJECT_NAME}.nro ALL
    DEPENDS ${PROJECT_NAME}
    COMMAND ${NX_NACPTOOL_EXE} --create "${PROJECT_NAME}" "${PROJECT_AUTHOR}" "${APP_VERSION}" ${PROJECT_NAME}.nacp
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_RESOURCES} ${CMAKE_BINARY_DIR}/resources
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/resources/font/*.txt
    COMMAND ${NX_ELF2NRO_EXE} ${PROJECT_NAME}.elf ${PROJECT_NAME}.nro --icon=${PROJECT_ICON}
        --nacp=${PROJECT_NAME}.nacp --romfsdir=${CMAKE_BINARY_DIR}/resources
)

target_include_directories(${PROJECT_NAME} PRIVATE darktube/include ${APP_PLATFORM_INCLUDE})
target_compile_options(${PROJECT_NAME} PRIVATE -ffunction-sections -fdata-sections -Wunused-variable ${APP_PLATFORM_OPTION})
target_link_libraries(${PROJECT_NAME} PRIVATE borealis ${APP_PLATFORM_LIB})
target_link_options(${PROJECT_NAME} PRIVATE ${APP_PLATFORM_LINK_OPTION} "-Wl,--allow-multiple-definition")
