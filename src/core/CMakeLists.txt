set(CORE_SOURCES
    cover_art.cpp
    io.cpp
    mpris.cpp
    queue_element.cpp
    scanner.cpp
    track_file.cpp
    musicdb/collection.cpp
    musicdb/musicdb.cpp
    musicdb/playlist.cpp
    musicdb/track.cpp
    player/audio_engine.cpp
    player/equalizer.cpp
    player/player.cpp
    player/queue.cpp
    ${CMAKE_SOURCE_DIR}/lib/miniaudio/miniaudio.c
)

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
    ${CMAKE_SOURCE_DIR}/lib/taglib/taglib/mpeg
    ${CMAKE_SOURCE_DIR}/lib/taglib/taglib/mpeg/id3v2
    ${CMAKE_SOURCE_DIR}/lib/taglib/taglib/mpeg/id3v2/frames
    ${CMAKE_SOURCE_DIR}/lib/taglib/taglib/ogg
    ${CMAKE_SOURCE_DIR}/lib/taglib/taglib/flac
    ${CMAKE_SOURCE_DIR}/lib/taglib/taglib/mp4
    ${CMAKE_BINARY_DIR}/lib/taglib
    ${CMAKE_SOURCE_DIR}/lib/glaze/include
    ${CMAKE_SOURCE_DIR}/lib/fmt/include
    ${CMAKE_SOURCE_DIR}/lib/glm
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
