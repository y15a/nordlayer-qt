cmake_minimum_required(VERSION 3.20)
project(nordlayer-qt VERSION 0.2.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(nordlayer-qt
    src/main.cpp
    src/mainwindow.cpp
    src/nordlayerclient.cpp
    src/nordlayerparser.cpp
    src/systemtraymanager.cpp
    resources/resources.qrc
)

target_include_directories(nordlayer-qt PRIVATE src)
target_link_libraries(nordlayer-qt PRIVATE Qt6::Widgets)
target_compile_definitions(nordlayer-qt PRIVATE
    APP_VERSION="${PROJECT_VERSION}")

# Install
include(GNUInstallDirs)
option(INSTALL_AUTOSTART "Install XDG autostart entry" ON)

# Autostart directory: /etc/xdg/autostart for system installs,
# ~/.config/autostart for user installs (override with -DAUTOSTART_INSTALL_DIR=...)
if(NOT DEFINED CACHE{AUTOSTART_INSTALL_DIR})
    if(CMAKE_INSTALL_PREFIX MATCHES "^$ENV{HOME}")
        set(_default_autostart "$ENV{HOME}/.config/autostart")
    else()
        set(_default_autostart "${CMAKE_INSTALL_FULL_SYSCONFDIR}/xdg/autostart")
    endif()
    set(AUTOSTART_INSTALL_DIR "${_default_autostart}"
        CACHE PATH "Directory for the XDG autostart .desktop file")
endif()

set(NORDLAYER_QT_BINARY "${CMAKE_INSTALL_FULL_BINDIR}/nordlayer-qt")

configure_file(
    resources/nordlayer-qt.desktop.in
    "${CMAKE_CURRENT_BINARY_DIR}/nordlayer-qt.desktop"
    @ONLY)

configure_file(
    resources/nordlayer-qt-autostart.desktop.in
    "${CMAKE_CURRENT_BINARY_DIR}/nordlayer-qt-autostart.desktop"
    @ONLY)

install(TARGETS nordlayer-qt RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/nordlayer-qt.desktop"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/applications")

if(INSTALL_AUTOSTART)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/nordlayer-qt-autostart.desktop"
            DESTINATION "${AUTOSTART_INSTALL_DIR}")
endif()

install(FILES resources/icons/nordlayer-qt.svg
        DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps")

# Tests
option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
    enable_testing()
    find_package(Qt6 REQUIRED COMPONENTS Test)
    add_subdirectory(tests)
endif()
