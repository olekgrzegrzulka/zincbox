cmake_minimum_required(VERSION 3.10)
project(zincbox VERSION 0.0.0 LANGUAGES CXX C)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

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

if(NOT WIN32)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(SYSTEMD REQUIRED libsystemd)
  add_subdirectory(lib/sdbus-cpp SYSTEM)
endif()

# FIXME properly add TagLib :)
include_directories(SYSTEM
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/taglib
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/taglib/taglib/mpeg/id3v2
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/taglib/taglib/ogg
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/taglib/taglib/flac
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/taglib/taglib/mp4
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/taglib/taglib
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/taglib/taglib/toolkit
)


set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(OUTPUT_NAME "${PROJECT_NAME}")

message(STATUS "------------------------------------------------")
message(STATUS " Project Name:    ${PROJECT_NAME}")
message(STATUS " C++ Standard:    ${CMAKE_CXX_STANDARD}")
message(STATUS " Compiler ID:     ${CMAKE_CXX_COMPILER_ID}")
message(STATUS " Compiler Path:   ${CMAKE_CXX_COMPILER}")
message(STATUS " Build Type:      ${CMAKE_BUILD_TYPE}")
message(STATUS " ASAN Enabled:    ${ENABLE_ASAN}")
if(MY_FLAGS)
  message(STATUS " Extra Flags:     ${MY_FLAGS}")
endif()
message(STATUS "------------------------------------------------")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
add_subdirectory(lib/freetype SYSTEM)
add_subdirectory(lib/glfw SYSTEM)
add_subdirectory(lib/nativefiledialog-extended SYSTEM)
add_subdirectory(lib/zlib SYSTEM)
set(ZLIB_FOUND TRUE)
set(ZLIB_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/lib/zlib)
set(ZLIB_LIBRARIES zlibstatic)
if(NOT TARGET ZLIB::ZLIB)
  add_library(ZLIB::ZLIB ALIAS zlibstatic)
endif()
add_subdirectory(lib/libspng SYSTEM)
add_subdirectory(lib/taglib SYSTEM)
add_subdirectory(lib/glm SYSTEM)
add_subdirectory(lib/stb_image SYSTEM)
add_subdirectory(lib/glad SYSTEM)
add_subdirectory(lib/miniz SYSTEM)

# add_subdirectory(lib/miniaudio SYSTEM)


set(SOURCES
    ./src/main.cpp
    ./lib/miniaudio/miniaudio.c
    ./src/common/config.cpp
    ./src/common/input.cpp
    ./src/core/musicdb/musicdb.cpp
    ./src/core/musicdb/collection.cpp
    ./src/core/musicdb/playlist.cpp
    ./src/core/musicdb/track.cpp
    ./src/core/player.cpp
    ./src/core/mpris.cpp
    ./src/core/io.cpp
    ./src/ui_generic/texture.cpp
    ./src/ui_generic/texture_atlas.cpp
    ./src/ui_generic/shader.cpp
    ./src/ui_generic/ui.cpp
    ./src/ui_generic/font_face.cpp
    ./src/ui_generic/button.cpp
    ./src/ui_generic/label.cpp
    ./src/ui_generic/slider.cpp
    ./src/ui_generic/sprite.cpp
    ./src/ui_generic/widget.cpp
    ./src/ui_generic/text_input.cpp
    ./src/ui_generic/selectbar.cpp
    ./src/ui_generic/scrollbar.cpp
    ./src/ui/interface.cpp
    ./src/ui/panel_albums.cpp
    ./src/ui/panel_controls.cpp
    ./src/ui/panel_tracks.cpp
    ./src/ui/panel_queue.cpp
    ./src/ui/panel_top.cpp
    ./src/ui/theme.cpp
    ./src/ui/popup_controller.cpp
    ./src/ui/popup.cpp
    ./src/ui/tab_bar.cpp
)

add_executable(${OUTPUT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    -pedantic
    -Werror=shadow
    -Werror=switch
    -Werror=unused-result
    -Werror=return-type
    -fPIC
    -Wno-narrowing
)

option(ENABLE_ASAN "Enable AddressSanitizer" OFF)

if(ENABLE_ASAN)
  message(STATUS "AddressSanitizer enabled")
  target_compile_options(${PROJECT_NAME} PRIVATE
    -fsanitize=address,undefined
    -fno-omit-frame-pointer
    -g
    -O1
    -D_GLIBCXX_ASSERTIONS
  )
  target_link_options(${PROJECT_NAME} PRIVATE
  ${SANITIZER_FLAGS}
    -fsanitize=address,undefined
    -fno-omit-frame-pointer
    -g
  )
else()
  message(STATUS "AddressSanitizer disabled")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_options(${PROJECT_NAME} PRIVATE -g -O1)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
  target_compile_options(${PROJECT_NAME} PRIVATE -O3 -DNDEBUG)
endif()




target_link_libraries(${PROJECT_NAME} PRIVATE freetype)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)
target_link_libraries(${PROJECT_NAME} PRIVATE nfd)
target_link_libraries(${PROJECT_NAME} PRIVATE zlibstatic)
target_link_libraries(${PROJECT_NAME} PRIVATE spng_static)
target_link_libraries(${PROJECT_NAME} PRIVATE tag)
if(NOT WIN32)
  target_link_libraries(${PROJECT_NAME} PRIVATE sdbus-c++)
endif()
target_link_libraries(${PROJECT_NAME} PRIVATE glm::glm)
target_link_libraries(${PROJECT_NAME} PRIVATE stb_image)
target_link_libraries(${PROJECT_NAME} PRIVATE glad)
target_link_libraries(${PROJECT_NAME} PRIVATE miniz)
# target_link_libraries(${PROJECT_NAME} PRIVATE miniaudio)
