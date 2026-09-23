cmake_minimum_required(VERSION 3.10)
project(zincbox VERSION 0.0.0 LANGUAGES CXX C)

option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ZINCBOX_GUI "Build the graphical interface" ON)

if(WIN32)
  set(ZINCBOX_MPRIS OFF CACHE BOOL "Build MPRIS support" FORCE)
else()
  option(ZINCBOX_MPRIS "Build MPRIS support" ON)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(OUTPUT_NAME "${PROJECT_NAME}")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(BUILD_TESTS OFF)

add_compile_definitions(
    $<$<BOOL:${ZINCBOX_GUI}>:ZINCBOX_HAS_GUI>
    $<$<BOOL:${ZINCBOX_MPRIS}>:ZINCBOX_HAS_MPRIS>
)

add_subdirectory(lib)
add_subdirectory(src/core)

if(ZINCBOX_GUI)
  add_subdirectory(src/ui)
endif()

set(MAIN_SOURCES
    src/main.cpp
    src/zincbox.cpp
    src/common/logger.cpp
)

add_executable(${OUTPUT_NAME} ${MAIN_SOURCES})

target_include_directories(${OUTPUT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${OUTPUT_NAME} PRIVATE zincbox_core)
if(ZINCBOX_GUI)
  target_link_libraries(${OUTPUT_NAME} PRIVATE zincbox_ui)
endif()

# Compiler options
if(MSVC)
  target_compile_options(${OUTPUT_NAME} PRIVATE
        /W4 /we4456 /we4062 /we4834 /we4715 /wd4838 /wd4244 /utf-8 /Zc:__cplusplus
    )
else()
  target_compile_options(${OUTPUT_NAME} PRIVATE
        -Wall -Wextra -pedantic -Werror=shadow -Werror=switch
        -Werror=unused-result -Werror=return-type -Wno-narrowing -fdiagnostics-color=always
        $<$<NOT:$<PLATFORM_ID:Windows>>:-fPIC>
    )
endif()

if(ENABLE_ASAN)
  if(MSVC)
    target_compile_options(${OUTPUT_NAME} PRIVATE /fsanitize=address /Oy- /Zi /Od)
  else()
    target_compile_options(${OUTPUT_NAME} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer -g -O1 -D_GLIBCXX_ASSERTIONS)
    target_link_options(${OUTPUT_NAME} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer -g)
  endif()
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  if(MSVC)
    target_compile_options(${OUTPUT_NAME} PRIVATE /Zi /Od)
  else()
    target_compile_options(${OUTPUT_NAME} PRIVATE -g -O1)
  endif()
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
  if(MSVC)
    target_compile_options(${OUTPUT_NAME} PRIVATE /O2 /DNDEBUG)
  else()
    target_compile_options(${OUTPUT_NAME} PRIVATE -O3 -DNDEBUG)
  endif()
endif()

# STD/System dependencies
target_link_libraries(${OUTPUT_NAME} PRIVATE
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:stdc++exp>
)
