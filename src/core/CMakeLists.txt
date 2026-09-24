set(CORE_SOURCES
    musicdb/musicdb.cpp
    musicdb/collection.cpp
    musicdb/playlist.cpp
    musicdb/track.cpp
    player.cpp
    io.cpp
    track_file.cpp
    scanner.cpp
    ${CMAKE_SOURCE_DIR}/lib/miniaudio/miniaudio.c
)

if(ZINCBOX_MPRIS)
    list(APPEND CORE_SOURCES mpris.cpp)
endif()

add_library(zincbox_core STATIC ${CORE_SOURCES})

target_include_directories(zincbox_core PUBLIC 
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

# TagLib system includes
target_include_directories(zincbox_core SYSTEM PUBLIC
    ${CMAKE_SOURCE_DIR}/lib/taglib
    ${CMAKE_SOURCE_DIR}/lib/taglib/taglib
    ${CMAKE_SOURCE_DIR}/lib/taglib/taglib/toolkit
    ${CMAKE_SOURCE_DIR}/lib/taglib/taglib/mpeg/id3v2
    ${CMAKE_SOURCE_DIR}/lib/taglib/taglib/mpeg/id3v2/frames
    ${CMAKE_SOURCE_DIR}/lib/taglib/taglib/ogg
    ${CMAKE_SOURCE_DIR}/lib/taglib/taglib/flac
    ${CMAKE_SOURCE_DIR}/lib/taglib/taglib/mp4
    ${CMAKE_SOURCE_DIR}/lib/glaze/include
    ${CMAKE_SOURCE_DIR}/lib/fmt/include
)

target_link_libraries(zincbox_core PUBLIC
    tag
    glaze::glaze
    miniz::miniz
    fmt::fmt
)

if(ZINCBOX_MPRIS)
    target_link_libraries(zincbox_core PUBLIC sdbus-c++)
endif()