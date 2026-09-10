cmake_minimum_required(VERSION 3.19)

project(miqumusic VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-O3 -Wall -Wextra -Wno-unused-parameter)

find_package(PkgConfig REQUIRED)
pkg_check_modules(DEPS REQUIRED IMPORTED_TARGET miqutoolkit libmpdclient fontconfig pangocairo cairo)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(EMBEDDED_FONT_CPP "${CMAKE_CURRENT_BINARY_DIR}/generated/EmbeddedFont.cpp")
set(EMBEDDED_ARTWORK_CPP "${CMAKE_CURRENT_BINARY_DIR}/generated/EmbeddedArtwork.cpp")

add_custom_command(
    OUTPUT ${EMBEDDED_FONT_CPP}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_asset.py
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/miqumusic.ttf
            ${EMBEDDED_FONT_CPP}
            UI::Components g_miqumusicFontData
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/miqumusic.ttf
            ${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_asset.py
    COMMENT "Generating EmbeddedFont.cpp from assets/fonts/miqumusic.ttf"
)

add_custom_command(
    OUTPUT ${EMBEDDED_ARTWORK_CPP}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_asset.py
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/default_album_art.png
            ${EMBEDDED_ARTWORK_CPP}
            Utils g_defaultAlbumArtData
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/assets/default_album_art.png
            ${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_asset.py
    COMMENT "Generating EmbeddedArtwork.cpp from assets/default_album_art.png"
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}/generated
)

set(SOURCES
    src/main.cpp
    src/core/config.cpp
    src/core/app.cpp
    src/mpd/mpd_client.cpp
    src/mpd/mpd_manager.cpp
    src/utils/icon_provider.cpp
    src/utils/artwork_utils.cpp
    src/views/content_section.cpp
    src/views/control_section.cpp
    src/views/custom_seek_bar.cpp
    src/views/playback_bar.cpp
    src/views/player_view.cpp
    src/views/queue_view.cpp
    src/views/database_view.cpp
    src/views/playlists_view.cpp
    src/views/visualizer_view.cpp
    src/views/ytdlp_view.cpp
    src/views/settings_view.cpp
    ${EMBEDDED_FONT_CPP}
    ${EMBEDDED_ARTWORK_CPP}
)

add_executable(miqumusic ${SOURCES})

target_link_libraries(miqumusic PRIVATE PkgConfig::DEPS pthread)

include(GNUInstallDirs)
install(TARGETS miqumusic RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES assets/miqumusic.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES assets/miqumusic.conf DESTINATION ${CMAKE_INSTALL_DATADIR}/miqumusic)
