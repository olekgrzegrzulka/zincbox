add_library(glad STATIC glad.c)

target_include_directories(glad
    SYSTEM PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
)
