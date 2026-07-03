cmake_minimum_required(VERSION 3.16)
project(UplinkIRC VERSION 0.25.55 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

option(UPLINK_VENDOR_DEPS "Download missing dependencies automatically" ON)

if(WIN32)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Svg WebSockets)
find_package(Qt6Keychain QUIET)
if(NOT Qt6Keychain_FOUND)
    if(NOT UPLINK_VENDOR_DEPS)
        message(FATAL_ERROR "Qt6Keychain not found. Install qtkeychain-qt6 or set -DUPLINK_VENDOR_DEPS=ON")
    endif()
    include(FetchContent)
    FetchContent_Declare(qtkeychain
        GIT_REPOSITORY https://github.com/frankosterfeld/qtkeychain.git
        GIT_TAG        38ff50766c7e4ec490fecb2d9939b7cdc4b74b8e  # 0.14.3
    )
    set(BUILD_WITH_QT6 ON CACHE BOOL "" FORCE)
    set(QT_DEFAULT_MAJOR_VERSION 6 CACHE STRING "" FORCE)
    set(BUILD_TRANSLATIONS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(qtkeychain)
    # qtkeychain 0.14.x creates qt6keychain in the build tree, not the
    # namespaced alias that only appears in installed CMake config files
    if(NOT TARGET Qt6Keychain::Qt6Keychain)
        foreach(_t qt6keychain qtkeychain)
            if(TARGET ${_t})
                add_library(Qt6Keychain::Qt6Keychain ALIAS ${_t})
                break()
            endif()
        endforeach()
    endif()
    # The source tree puts keychain.h at its root; installed packages put it
    # under qt6keychain/. Generate a forwarding header to match the installed
    # layout so #include <qt6keychain/keychain.h> resolves from either path.
    FetchContent_GetProperties(qtkeychain SOURCE_DIR _qtkc_src BINARY_DIR _qtkc_bin)
    set(_qtkc_compat "${CMAKE_BINARY_DIR}/qt6keychain-compat")
    file(MAKE_DIRECTORY "${_qtkc_compat}/qt6keychain")
    file(WRITE "${_qtkc_compat}/qt6keychain/keychain.h"
        "#pragma once\n#include \"${_qtkc_src}/keychain.h\"\n")
    # _qtkc_bin holds qkeychain_export.h (generated at configure time by CMake)
    set(QTKEYCHAIN_COMPAT_INCLUDE "${_qtkc_compat}" "${_qtkc_bin}")
endif()
find_package(tomlplusplus QUIET)
if(NOT tomlplusplus_FOUND)
    if(NOT UPLINK_VENDOR_DEPS)
        message(FATAL_ERROR "tomlplusplus not found. Install tomlplusplus or set -DUPLINK_VENDOR_DEPS=ON")
    endif()
    include(FetchContent)
    FetchContent_Declare(tomlplusplus
        GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
        GIT_TAG        30172438cee64926dc41fdd9c11fb3ba5b2ba9de  # v3.4.0
    )
    FetchContent_MakeAvailable(tomlplusplus)
endif()
qt_standard_project_setup()

qt_add_executable(Uplink WIN32
    resources/resources.qrc
    src/main.cpp
    src/config/config.cpp
    src/config/config.h
    src/config/keychainhelper.cpp
    src/config/keychainhelper.h
    src/model/sessionmodel.cpp
    src/model/sessionmodel.h
    src/model/serversession.h
    src/model/channel.h
    src/model/message.h
    src/ui/mainwindow.cpp
    src/ui/mainwindow.h
    src/ui/contextmenus.cpp
    src/ui/nickpanel.cpp
    src/ui/commanddispatcher.cpp
    src/ui/commanddispatcher.h
    src/ui/chatline.h
    src/ui/chatrenderer.cpp
    src/ui/chatrenderer.h
    src/ui/chatview.cpp
    src/ui/chatview.h
    src/ui/channelpane.cpp
    src/ui/channelpane.h
    src/ui/trayicon.cpp
    src/ui/trayicon.h
    src/ui/aboutdialog.cpp
    src/ui/aboutdialog.h
    src/ui/channellistdialog.cpp
    src/ui/channellistdialog.h
    src/ui/docsdialog.cpp
    src/ui/docsdialog.h
    src/ui/fontdialog.cpp
    src/ui/fontdialog.h
    src/ui/serverdialog.cpp
    src/ui/serverdialog.h
    src/ui/manageserversdialog.cpp
    src/ui/manageserversdialog.h
    src/ui/preferencesdialog.cpp
    src/ui/preferencesdialog.h
    src/ui/appicons.h
    src/ui/emojidata.h
    src/ui/emojipicker.h
    src/ui/emojipicker.cpp
    src/ui/linkpreview.cpp
    src/ui/linkpreview.h
    src/ui/fadescrollbar.cpp
    src/ui/fadescrollbar.h
    src/ui/signalbars.cpp
    src/ui/signalbars.h
    src/ui/quickswitcher.cpp
    src/ui/quickswitcher.h
    src/ui/updatechecker.cpp
    src/ui/updatechecker.h
    src/ui/dcccontroller.cpp
    src/ui/dcccontroller.h
    src/ui/previewcontroller.cpp
    src/ui/previewcontroller.h
    src/ui/themeloader.cpp
    src/ui/themeloader.h
    src/irc/ircclient.cpp
    src/irc/ircclient.h
    src/irc/ircparser.cpp
    src/irc/ircparser.h
    src/irc/stsstore.cpp
    src/irc/stsstore.h
    src/irc/dccsend.cpp
    src/irc/dccsend.h
    src/irc/dccreceive.cpp
    src/irc/dccreceive.h
)

configure_file(src/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/version.h)
target_include_directories(Uplink PRIVATE src ${CMAKE_CURRENT_BINARY_DIR} ${QTKEYCHAIN_COMPAT_INCLUDE})

target_link_libraries(Uplink PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Svg
    Qt6::WebSockets
    Qt6Keychain::Qt6Keychain
    tomlplusplus::tomlplusplus
)

# Prevent .relr.dyn sections that linuxdeploy's bundled strip cannot handle
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_link_options(Uplink PRIVATE -Wl,-z,nopack-relative-relocs)
endif()

# Compiler warnings
option(UPLINK_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
option(UPLINK_ENABLE_SANITIZERS  "Enable ASan/UBSan"        OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(Uplink PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion
        -Wshadow -Wnon-virtual-dtor -Wold-style-cast
    )
    if(UPLINK_WARNINGS_AS_ERRORS)
        target_compile_options(Uplink PRIVATE -Werror)
    endif()
endif()

if(UPLINK_ENABLE_SANITIZERS AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(Uplink PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(Uplink PRIVATE -fsanitize=address,undefined)
endif()

# Copy themes next to binary for development
add_custom_command(TARGET Uplink POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/themes $<TARGET_FILE_DIR:Uplink>/themes
)

if(APPLE)
    set(UPLINK_ICONSET_DIR "${CMAKE_CURRENT_BINARY_DIR}/uplink.iconset")
    set(UPLINK_ICNS        "${CMAKE_CURRENT_BINARY_DIR}/uplink.icns")
    set(UPLINK_ICON_SRC    "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/uplink-dark.png")
    add_custom_command(
        OUTPUT "${UPLINK_ICNS}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${UPLINK_ICONSET_DIR}"
        COMMAND sips -z 16  16  "${UPLINK_ICON_SRC}" --out "${UPLINK_ICONSET_DIR}/icon_16x16.png"
        COMMAND sips -z 32  32  "${UPLINK_ICON_SRC}" --out "${UPLINK_ICONSET_DIR}/icon_16x16@2x.png"
        COMMAND sips -z 32  32  "${UPLINK_ICON_SRC}" --out "${UPLINK_ICONSET_DIR}/icon_32x32.png"
        COMMAND sips -z 64  64  "${UPLINK_ICON_SRC}" --out "${UPLINK_ICONSET_DIR}/icon_32x32@2x.png"
        COMMAND sips -z 128 128 "${UPLINK_ICON_SRC}" --out "${UPLINK_ICONSET_DIR}/icon_128x128.png"
        COMMAND sips -z 256 256 "${UPLINK_ICON_SRC}" --out "${UPLINK_ICONSET_DIR}/icon_128x128@2x.png"
        COMMAND sips -z 256 256 "${UPLINK_ICON_SRC}" --out "${UPLINK_ICONSET_DIR}/icon_256x256.png"
        COMMAND sips -z 512 512 "${UPLINK_ICON_SRC}" --out "${UPLINK_ICONSET_DIR}/icon_256x256@2x.png"
        COMMAND sips -z 512 512 "${UPLINK_ICON_SRC}" --out "${UPLINK_ICONSET_DIR}/icon_512x512.png"
        COMMAND iconutil -c icns "${UPLINK_ICONSET_DIR}" -o "${UPLINK_ICNS}"
        DEPENDS "${UPLINK_ICON_SRC}"
        COMMENT "Generating macOS app icon"
        VERBATIM
    )
    set_source_files_properties("${UPLINK_ICNS}" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    target_sources(Uplink PRIVATE "${UPLINK_ICNS}")
    set_target_properties(Uplink PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_ICON_FILE uplink.icns
    )
endif()

install(TARGETS Uplink DESTINATION bin)
install(DIRECTORY themes/ DESTINATION share/uplink/themes)
install(FILES packaging/Uplink.desktop DESTINATION share/applications)
install(FILES packaging/io.github.noderelay.UplinkIRC.metainfo.xml DESTINATION share/metainfo)
install(FILES resources/icons/uplink-dark.png
        DESTINATION share/icons/hicolor/256x256/apps
        RENAME uplink.png)

option(UPLINK_BUILD_TESTS "Build unit tests" ON)
if(UPLINK_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
