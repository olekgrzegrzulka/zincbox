cmake_minimum_required(VERSION 3.10)

# GLFW
set(GLFW_STANDALONE ON CACHE BOOL "" FORCE)

# Native File Dialog Extended
option(NFD_PORTAL "" ON)
set(NFD_PORTAL ON)

# zlib
set(ZLIB_BUILD_TESTING  OFF)
set(ZLIB_BUILD_SHARED OFF)
set(ZLIB_BUILD_STATIC ON)
set(ZLIB_INSTALL ON)
set(ZLIB_PREFIX OFF)

# Freetype
set(BUILD_SHARED_LIBS OFF)

# libspng
set(SPNG_SHARED OFF)
set(SPNG_STATIC ON)
set(BUILD_EXAMPLES OFF)

#TagLib
set(BUILD_SHARED_LIBS OFF)
set(BUILD_EXAMPLES OFF)
set(BUILD_BINDINGS OFF)
set(NO_ITUNES_HACKS ON)
set(BUILD_TESTING OFF)

# FIXME properly add TagLib :)
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/taglib
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/taglib/taglib/mpeg/id3v2
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/taglib/taglib/ogg
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/taglib/taglib/flac
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/taglib/taglib/mp4
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/taglib/taglib
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/taglib/taglib/toolkit
)

project(music VERSION 0.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(OUTPUT_NAME "${PROJECT_NAME}")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
add_subdirectory(lib/freetype SYSTEM)
add_subdirectory(lib/glfw SYSTEM)
add_subdirectory(lib/nativefiledialog-extended SYSTEM)
add_subdirectory(lib/zlib SYSTEM)
add_subdirectory(lib/libspng SYSTEM)
add_subdirectory(lib/taglib SYSTEM)
# add_subdirectory(lib/miniaudio SYSTEM)


set(SOURCES
    ./src/main.cpp
    ./src/config.cpp
    ./src/input.cpp
    ./src/texture.cpp
    ./src/texture_atlas.cpp
    ./src/shader.cpp

    ./src/ui/ui.cpp
    ./src/ui/font_face.cpp
    ./src/ui/button.cpp
    ./src/ui/label.cpp
    ./src/ui/slider.cpp
    ./src/ui/sprite.cpp
    ./src/ui/widget.cpp
    ./src/ui/text_input.cpp
    ./src/ui/selectbar.cpp
    ./src/ui/scrollbar.cpp

    ./src/core/musicdb.cpp
    ./src/core/player.cpp
    ./src/interface.cpp

    ./lib/glad/glad.c
    ./lib/miniaudio/miniaudio.c
)

add_executable(${OUTPUT_NAME} ${SOURCES})

target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    -pedantic
    -Werror=shadow
    -Werror=switch
    -Werror=unused-result
    -Werror=return-type
)

target_compile_options(${PROJECT_NAME} PRIVATE -O3)

# target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address)
# target_link_options(${PROJECT_NAME} PRIVATE -fno-omit-frame-pointer)
# target_link_options(${PROJECT_NAME} PRIVATE -g)

target_link_libraries(${PROJECT_NAME} PRIVATE freetype)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)
target_link_libraries(${PROJECT_NAME} PRIVATE nfd)
target_link_libraries(${PROJECT_NAME} PRIVATE zlibstatic)
target_link_libraries(${PROJECT_NAME} PRIVATE spng_static)
target_link_libraries(${PROJECT_NAME} PRIVATE tag)
# target_link_libraries(${PROJECT_NAME} PRIVATE miniaudio)
