add_library(xxHash STATIC xxhash.c)

target_include_directories(xxHash
    SYSTEM PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
)
