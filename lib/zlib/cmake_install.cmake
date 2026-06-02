# Install script for directory: /home/olek/Programowanie/redrawn-nouveau/lib/zlib

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "0")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set path to fallback-tool for dependency-resolution.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Development" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib64" TYPE STATIC_LIBRARY FILES "/home/olek/Programowanie/redrawn-nouveau/lib/zlib/libz.a")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/cmake/zlib/ZLIB-static.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/cmake/zlib/ZLIB-static.cmake"
         "/home/olek/Programowanie/redrawn-nouveau/lib/zlib/CMakeFiles/Export/a524ad2b1477c84c90b72316758fce2f/ZLIB-static.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/cmake/zlib/ZLIB-static-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib64/cmake/zlib/ZLIB-static.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib64/cmake/zlib" TYPE FILE FILES "/home/olek/Programowanie/redrawn-nouveau/lib/zlib/CMakeFiles/Export/a524ad2b1477c84c90b72316758fce2f/ZLIB-static.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib64/cmake/zlib" TYPE FILE FILES "/home/olek/Programowanie/redrawn-nouveau/lib/zlib/CMakeFiles/Export/a524ad2b1477c84c90b72316758fce2f/ZLIB-static-release.cmake")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib64/cmake/zlib" TYPE FILE FILES
    "/home/olek/Programowanie/redrawn-nouveau/lib/zlib/ZLIBConfig.cmake"
    "/home/olek/Programowanie/redrawn-nouveau/lib/zlib/ZLIBConfigVersion.cmake"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Development" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES
    "/home/olek/Programowanie/redrawn-nouveau/lib/zlib/zconf.h"
    "/home/olek/Programowanie/redrawn-nouveau/lib/zlib/zlib.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Docs" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/man/man3" TYPE FILE FILES "/home/olek/Programowanie/redrawn-nouveau/lib/zlib/zlib.3")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Docs" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/doc/zlib/zlib" TYPE FILE FILES
    "/home/olek/Programowanie/redrawn-nouveau/lib/zlib/LICENSE"
    "/home/olek/Programowanie/redrawn-nouveau/lib/zlib/doc/algorithm.txt"
    "/home/olek/Programowanie/redrawn-nouveau/lib/zlib/doc/crc-doc.1.0.pdf"
    "/home/olek/Programowanie/redrawn-nouveau/lib/zlib/doc/rfc1950.txt"
    "/home/olek/Programowanie/redrawn-nouveau/lib/zlib/doc/rfc1951.txt"
    "/home/olek/Programowanie/redrawn-nouveau/lib/zlib/doc/rfc1952.txt"
    "/home/olek/Programowanie/redrawn-nouveau/lib/zlib/doc/txtvsbin.txt"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Development" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib64/pkgconfig" TYPE FILE FILES "/home/olek/Programowanie/redrawn-nouveau/lib/zlib/zlib.pc")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/home/olek/Programowanie/redrawn-nouveau/lib/zlib/contrib/cmake_install.cmake")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/home/olek/Programowanie/redrawn-nouveau/lib/zlib/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
