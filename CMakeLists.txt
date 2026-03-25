cmake_minimum_required(VERSION 3.20)
project(nordlayer-qt VERSION 0.2.0 LANGUAGES CXX)

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

# Install
install(TARGETS nordlayer-qt RUNTIME DESTINATION bin)
install(FILES resources/nordlayer-qt.desktop
        DESTINATION share/applications)
install(FILES resources/nordlayer-qt-autostart.desktop
        DESTINATION share/applications)
install(FILES resources/icons/nordlayer-qt.svg
        DESTINATION share/icons/hicolor/scalable/apps)

# Tests
option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
    enable_testing()
    find_package(Qt6 REQUIRED COMPONENTS Test)
    add_subdirectory(tests)
endif()
