cmake_minimum_required(VERSION 3.10)
project(zincbox VERSION 0.0.0 LANGUAGES CXX C)

option(ENABLE_ASAN "Enable AddressSanitizer" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(OUTPUT_NAME "${PROJECT_NAME}")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(BUILD_TESTS OFF)

set(WITH_ZLIB OFF)
set(CMAKE_DISABLE_FIND_PACKAGE_ZLIB TRUE)

# Native File Dialog Extended
option(NFD_PORTAL "" ON)
set(NFD_PORTAL ON)
set(utf8cpp_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/utfcpp/source" CACHE PATH "" FORCE)

# Freetype
set(BUILD_SHARED_LIBS OFF)
set(FT_REQUIRE_ZLIB OFF)

# libspng
set(SPNG_SHARED OFF)
set(SPNG_STATIC ON)
set(SPNG_USE_MINIZ ON)
set(BUILD_EXAMPLES OFF)

# TagLib
set(BUILD_SHARED_LIBS OFF)
set(BUILD_EXAMPLES OFF)
set(BUILD_BINDINGS OFF)
set(NO_ITUNES_HACKS ON)
set(BUILD_TESTING OFF)

# GLM
set(GLM_BUILD_TESTS OFF)

# sdbus-c++
set(SDBUSCPP_BUILD_DOCS OFF)
set(BUILD_SHARED_LIBS OFF)

# SDL3
set(SDL_STATIC ON)
set(SDL_SHARED OFF)
set(SDL_X11_XSCRNSAVER OFF)
set(SDL_X11_XTEST OFF)
set(SDL_TEST_LIBRARY OFF)
set(SDL_AUDIO OFF)
set(SDL_GPU OFF)
set(SDL_RENDER OFF)
set(SDL_CAMERA OFF)
set(SDL_JOYSTICK OFF)
set(SDL_HAPTIC OFF)
set(SDL_HIDAPI OFF)
set(SDL_POWER OFF)
set(SDL_SENSOR OFF)
set(SDL_DIALOG OFF)
set(SDL_NOTIFICATION OFF)

# fmt
set(FMT_MODULE OFF)

if(NOT WIN32)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(SYSTEMD REQUIRED libsystemd)
  add_subdirectory(lib/sdbus-cpp SYSTEM)
endif()

# FIXME properly add TagLib :)
include_directories(SYSTEM
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/taglib
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/taglib/taglib/mpeg/id3v2
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/taglib/taglib/mpeg/id3v2/frames
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/taglib/taglib/ogg
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/taglib/taglib/flac
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/taglib/taglib/mp4
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/taglib/taglib
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/taglib/taglib/toolkit
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/glaze/include
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/fmt/include
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/glad/include
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/nativefiledialog-extended/src/include/
)

message(STATUS "------------------------------------------------")
message(STATUS " Project Name:    ${PROJECT_NAME}")
message(STATUS " C++ Standard:    ${CMAKE_CXX_STANDARD}")
message(STATUS " Generator:       ${CMAKE_GENERATOR}")
message(STATUS " Compiler:        ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} (${CMAKE_CXX_COMPILER})")
message(STATUS " Build Type:      ${CMAKE_BUILD_TYPE}")
message(STATUS " ASAN Enabled:    ${ENABLE_ASAN}")
if(CMAKE_LINKER_TYPE)
  message(STATUS " Linker Type:     ${CMAKE_LINKER_TYPE}")
elseif(CMAKE_EXE_LINKER_FLAGS MATCHES "-fuse-ld=([^ ]+)")
  message(STATUS " Linker:          ${CMAKE_MATCH_1}")
else()
  message(STATUS " Linker:          ${CMAKE_LINKER}")
endif()
if(MY_FLAGS)
  message(STATUS " Extra Flags:     ${MY_FLAGS}")
endif()
message(STATUS "------------------------------------------------")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
add_subdirectory(lib/freetype SYSTEM)
add_subdirectory(lib/nativefiledialog-extended SYSTEM)
add_subdirectory(lib/taglib SYSTEM)
add_subdirectory(lib/glm SYSTEM)
add_subdirectory(lib/stb_image SYSTEM)
add_subdirectory(lib/glad SYSTEM)
add_subdirectory(lib/miniz SYSTEM)
add_subdirectory(lib/fmt EXCLUDE_FROM_ALL SYSTEM)
add_subdirectory(lib/SDL SYSTEM)
add_subdirectory(lib/glaze SYSTEM)
add_subdirectory(lib/cmrc SYSTEM)
add_subdirectory(lib/xxHash SYSTEM)
# add_subdirectory(lib/miniaudio SYSTEM)

if (TARGET miniz AND NOT TARGET miniz::miniz)
  add_library(miniz::miniz ALIAS miniz)
endif()

add_library(spng_static STATIC lib/libspng/spng/spng.c)
target_include_directories(spng_static PUBLIC lib/libspng/spng)
target_compile_definitions(spng_static PUBLIC SPNG_USE_MINIZ SPNG_STATIC)
target_link_libraries(spng_static PRIVATE miniz::miniz)


file(GLOB_RECURSE THEME_FILES "${CMAKE_CURRENT_SOURCE_DIR}/resources/theme/*")

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/resources/theme.zip
    COMMAND ${CMAKE_COMMAND} -E tar "cf" "${CMAKE_CURRENT_SOURCE_DIR}/resources/theme.zip" --format=zip "."
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resources/theme
    DEPENDS ${THEME_FILES}
)

cmrc_add_resource_library(zincbox_resources NAMESPACE zincbox_resources)
cmrc_add_resources(zincbox_resources WHENCE resources resources/theme.zip)

set(SOURCES
./src/main.cpp
    ./src/ui/sdl3_window.cpp
    ./lib/miniaudio/miniaudio.c
    ./src/ui/zincgui/input.cpp
    ./src/common/logger.cpp
    ./src/zincbox.cpp
    ./src/core/musicdb/musicdb.cpp
    ./src/core/musicdb/collection.cpp
    ./src/core/musicdb/playlist.cpp
    ./src/core/musicdb/track.cpp
    ./src/ui/tray.cpp
    ./src/ui/tr.cpp
    ./src/core/player.cpp
    ./src/core/mpris.cpp
    ./src/core/io.cpp
    ./src/core/track_file.cpp
    ./src/core/scanner.cpp
    ./src/ui/zincgui/texture.cpp
    ./src/ui/zincgui/texture_atlas.cpp
    ./src/ui/zincgui/shader.cpp
    ./src/ui/zincgui/ui.cpp
    ./src/ui/zincgui/font_face.cpp
    ./src/ui/zincgui/button.cpp
    ./src/ui/zincgui/label.cpp
    ./src/ui/zincgui/slider.cpp
    ./src/ui/zincgui/sprite.cpp
    ./src/ui/zincgui/widget.cpp
    ./src/ui/zincgui/text_input.cpp
    ./src/ui/zincgui/scrollbar.cpp
    ./src/ui/zincgui/color_rect.cpp
    ./src/ui/interface.cpp
    ./src/ui/interface_notifications.cpp
    ./src/ui/panel_albums.cpp
    ./src/ui/panel_controls.cpp
    ./src/ui/panel_tracks.cpp
    ./src/ui/panel_queue.cpp
    ./src/ui/panel_top.cpp
    ./src/ui/theme.cpp
    ./src/ui/popup_controller.cpp
    ./src/ui/popup.cpp
    ./src/ui/tab_bar.cpp
    ./src/ui/scrollable_view.cpp
)

add_executable(${OUTPUT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE
        /W4              # -Wall
        /we4456          # error on variable shadowing
        /we4062          # error on unhandled switch enum cases
        /we4834          # error on discarding [[nodiscard]] return values
        /we4715          # error on missing return statement
        /wd4838          # disable narrowing conversion warnings
        /wd4244          # disable precision loss warnings
        /utf-8           # UTF-8
        /Zc:__cplusplus  # report correct C++ standard in __cplusplus macro
    )
else()
  target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    -pedantic
    -Werror=shadow
    -Werror=switch
    -Werror=unused-result
    -Werror=return-type
    -Wno-narrowing
    -fdiagnostics-color=always
    $<$<NOT:$<PLATFORM_ID:Windows>>:-fPIC>
)
endif()

if(ENABLE_ASAN)
  if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
      /fsanitize=address
      /Oy-               # -fno-omit-frame-pointer
      /Zi                # -g
      /Od                # -O0
    )
  else()
    target_compile_options(${PROJECT_NAME} PRIVATE
      -fsanitize=address,undefined
      -fno-omit-frame-pointer
      -g
      -O1
      -D_GLIBCXX_ASSERTIONS
    )
    target_link_options(${PROJECT_NAME} PRIVATE
      -fsanitize=address,undefined
      -fno-omit-frame-pointer
      -g
    )
  endif()
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /Zi /Od)
  else()
    target_compile_options(${PROJECT_NAME} PRIVATE -g -O1)
  endif()
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
  if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /O2 /DNDEBUG)
  else()
    target_compile_options(${PROJECT_NAME} PRIVATE -O3 -DNDEBUG)
  endif()
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
  freetype nfd spng_static
  tag glm::glm stb_image
  glad miniz fmt::fmt xxHash
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:stdc++exp>
  $<$<NOT:$<PLATFORM_ID:Windows>>:sdbus-c++>
  SDL3::SDL3-static glaze::glaze zincbox_resources
)
# target_link_libraries(${PROJECT_NAME} PRIVATE miniaudio)
