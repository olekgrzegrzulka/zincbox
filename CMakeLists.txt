cmake_minimum_required(VERSION 3.10)

# GLFW
set(GLFW_STANDALONE ON CACHE BOOL "" FORCE)

# Native File Dialog Extended
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

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/taglib
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/taglib/taglib/mpeg/id3v2
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/taglib/taglib/ogg
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/taglib/taglib/flac
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/taglib/taglib/mp4
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/taglib/taglib
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/taglib/taglib/toolkit
    
    # ${CMAKE_CURRENT_BINARY_DIR}/lib/taglib
)

project(music VERSION 0.0.0)

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -Werror=shadow -Werror=switch -Werror=unused-result -Werror=return-type ${MY_FLAGS} -O3")

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

    ./src/musicdb.cpp
    ./src/musicdb_serialize.cpp

    ./lib/glad/glad.c
)

add_executable(${OUTPUT_NAME} ${SOURCES})
target_compile_definitions(${PROJECT_NAME} PUBLIC
    "PROJECT_VERSION=${PROJECT_VERSION}"
)
target_compile_definitions(${PROJECT_NAME} PRIVATE TAGLIB_STATIC)

target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    -pedantic
    -Werror=shadow
    -Werror=switch
    -Werror=unused-result
    -Werror=return-type
    # -fsanitize=address
    # -fno-omit-frame-pointer
    # -g
    # -O1
)
target_compile_options(${PROJECT_NAME} PRIVATE -O3)

# target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address)

target_link_libraries(${PROJECT_NAME} PRIVATE freetype)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)
target_link_libraries(${PROJECT_NAME} PRIVATE nfd)
target_link_libraries(${PROJECT_NAME} PRIVATE zlibstatic)
target_link_libraries(${PROJECT_NAME} PRIVATE spng_static)
target_link_libraries(${PROJECT_NAME} PRIVATE tag)


# target_include_directories(${PROJECT_NAME} PRIVATE 
#     ${CMAKE_CURRENT_SOURCE_DIR}/lib/taglib
#     ${CMAKE_CURRENT_SOURCE_DIR}/lib/taglib/taglib
#     ${CMAKE_CURRENT_BINARY_DIR}/lib/taglib
# )
# target_include_directories(${PROJECT_NAME} PRIVATE lib/taglib/taglib)