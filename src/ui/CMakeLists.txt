# Resources
file(GLOB_RECURSE THEME_FILES "${CMAKE_SOURCE_DIR}/resources/theme/*")
file(GLOB_RECURSE LANGUAGE_FILES "${CMAKE_SOURCE_DIR}/resources/lang/*")

add_custom_command(
    OUTPUT ${CMAKE_SOURCE_DIR}/resources/theme.zip
    COMMAND ${CMAKE_COMMAND} -E tar "cf" "${CMAKE_SOURCE_DIR}/resources/theme.zip" --format=zip "."
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/resources/theme
    DEPENDS ${THEME_FILES}
)

add_custom_command(
    OUTPUT ${CMAKE_SOURCE_DIR}/resources/lang.zip
    COMMAND ${CMAKE_COMMAND} -E tar "cf" "${CMAKE_SOURCE_DIR}/resources/lang.zip" --format=zip "."
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/resources/lang
    DEPENDS ${LANGUAGE_FILES}
)

cmrc_add_resource_library(zincbox_resources NAMESPACE zincbox_resources)
cmrc_add_resources(zincbox_resources
    WHENCE ${CMAKE_SOURCE_DIR}/resources
    ${CMAKE_SOURCE_DIR}/resources/theme.zip
    ${CMAKE_SOURCE_DIR}/resources/lang.zip
)

set(UI_SOURCES
    cover_art.cpp
    popup.cpp
    panel_controls.cpp
    panel_playlists.cpp
    panel_queue.cpp
    panel_top.cpp
    panel_tracklist.cpp
    popup_controller.cpp
    popup_definitions.cpp
    popup_settings.cpp
    scrollable_view.cpp
    sdl3_window.cpp
    splitter.cpp
    tab_bar.cpp
    theme.cpp
    tr.cpp
    tray.cpp
    ui_notifications.cpp
    ui.cpp
    widget_track.cpp
    zb_widgets.cpp
    zincgui/button.cpp
    zincgui/color_rect.cpp
    zincgui/combo_box.cpp
    zincgui/font_face.cpp
    zincgui/input.cpp
    zincgui/label.cpp
    zincgui/layout.cpp
    zincgui/root.cpp
    zincgui/scrollbar.cpp
    zincgui/shader.cpp
    zincgui/slider.cpp
    zincgui/spinner.cpp
    zincgui/sprite.cpp
    zincgui/text_input.cpp
    zincgui/texture_atlas.cpp
    zincgui/texture.cpp
    zincgui/tooltip.cpp
    zincgui/widget.cpp
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
    harfbuzz
    zincbox_resources
)
