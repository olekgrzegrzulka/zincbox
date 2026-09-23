# Resources
file(GLOB_RECURSE THEME_FILES "${CMAKE_SOURCE_DIR}/resources/theme/*")

add_custom_command(
    OUTPUT ${CMAKE_SOURCE_DIR}/resources/theme.zip
    COMMAND ${CMAKE_COMMAND} -E tar "cf" "${CMAKE_SOURCE_DIR}/resources/theme.zip" --format=zip "."
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/resources/theme
    DEPENDS ${THEME_FILES}
)

cmrc_add_resource_library(zincbox_resources NAMESPACE zincbox_resources)
cmrc_add_resources(zincbox_resources WHENCE ${CMAKE_SOURCE_DIR}/resources ${CMAKE_SOURCE_DIR}/resources/theme.zip)

set(UI_SOURCES
    sdl3_window.cpp
    tray.cpp
    tr.cpp
    interface.cpp
    interface_notifications.cpp
    panel_albums.cpp
    panel_controls.cpp
    panel_tracks.cpp
    panel_queue.cpp
    panel_top.cpp
    theme.cpp
    popup_controller.cpp
    popup.cpp
    tab_bar.cpp
    scrollable_view.cpp
    zincgui/input.cpp
    zincgui/texture.cpp
    zincgui/texture_atlas.cpp
    zincgui/shader.cpp
    zincgui/ui.cpp
    zincgui/font_face.cpp
    zincgui/button.cpp
    zincgui/label.cpp
    zincgui/slider.cpp
    zincgui/sprite.cpp
    zincgui/widget.cpp
    zincgui/text_input.cpp
    zincgui/scrollbar.cpp
    zincgui/color_rect.cpp
)

add_library(zincbox_ui STATIC ${UI_SOURCES})

target_include_directories(zincbox_ui PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

target_include_directories(zincbox_ui SYSTEM PUBLIC
    ${CMAKE_SOURCE_DIR}/lib/glad/include
    ${CMAKE_SOURCE_DIR}/lib/nativefiledialog-extended/src/include/
)

target_link_libraries(zincbox_ui PUBLIC
    zincbox_core
    freetype
    nfd
    spng_static
    glm::glm
    stb_image
    glad
    SDL3::SDL3-static
    zincbox_resources
)
