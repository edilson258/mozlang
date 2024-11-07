enable_testing()
include(GoogleTest)

add_executable(lexer_test lexer_test.cpp
  ${CMAKE_SOURCE_DIR}/src/DiagnosticEngine.cpp
  ${CMAKE_SOURCE_DIR}/src/Painter.cpp
  ${CMAKE_SOURCE_DIR}/src/Lexer.cpp)
target_link_libraries(lexer_test GTest::gtest_main)
gtest_discover_tests(lexer_test)
