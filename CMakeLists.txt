cmake_minimum_required(VERSION 3.8)

project (ZeroLang
  VERSION 0.1.0
  LANGUAGES CXX
  DESCRIPTION "..."
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(
  -Wall                 # Enable most warnings
  -Wextra               # Enable additional warnings
  -Werror               # Treat warnings as errors
  -Wshadow              # Warn when a variable declaration shadows another variable
  -Wnon-virtual-dtor    # Warn when a class with virtual functions does not have a virtual destructor
  -pedantic             # Enforce strict ISO C++ compliance
  -Wold-style-cast      # Warn on C-style casts
  -Wcast-align          # Warn on potential performance issues due to misaligned casts
  -Wunused              # Warn on unused variables, parameters, etc.
  -Woverloaded-virtual  # Warn when a base class function is hidden by a function with the same name in the derived class
  -Wconversion          # Warn on implicit conversions that may change a value
  -Wsign-conversion     # Warn on implicit conversions between signed and unsigned integers
  -Wnull-dereference    # Warn if a null pointer dereference is detected
)

file(GLOB_RECURSE zero_sources "src/*.cpp")
add_executable(zero ${zero_sources})
target_include_directories(zero PUBLIC ${CMAKE_SOURCE_DIR}/src)
