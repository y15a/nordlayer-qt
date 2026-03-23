add_executable(test_parser
    test_parser.cpp
    ${CMAKE_SOURCE_DIR}/src/nordlayerparser.cpp
)

target_include_directories(test_parser PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_parser PRIVATE Qt6::Test Qt6::Widgets)

add_test(NAME test_parser COMMAND test_parser)
