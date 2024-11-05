enable_testing()
include(GoogleTest)

file(GLOB TestSources "*.cpp")
foreach(TestSource ${TestSources})
  get_filename_component(TestFileName ${TestSource} NAME_WE)
  add_executable(${TestFileName} ${TestSource})

  target_link_libraries(${TestFileName} GTest::gtest_main)

  add_test(NAME ${TestFileName} COMMAND ${TestFileName})
  gtest_discover_tests(${TestFileName})
endforeach()
