cmake_minimum_required (VERSION 3.8)

project (Mozlang
    VERSION 0.1.0
    LANGUAGES C CXX
    DESCRIPTION "A programming language developed by Mozambicans"
)

cmake_policy(SET CMP0075 NEW)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(LLVM REQUIRED CONFIG)
list(APPEND CMAKE_MODULE_PATH ${LLVM_CMAKE_DIR})
include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})
include(AddLLVM)
add_definitions(${LLVM_DEFINITIONS})

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

file(GLOB_RECURSE moz_sources "src/*.cpp")
add_executable(mozlang ${moz_sources})

set(LLVM_LINK_COMPONENTS
  Core
  Analysis
  AsmParser
  BitReader
  BitWriter
  CodeGen
  ExecutionEngine
  IRReader
  LTO
  MC
  MCJIT
  Support
  nativecodegen
  Interpreter
)

llvm_map_components_to_libnames(llvm_libs ${LLVM_LINK_COMPONENTS})
target_link_libraries(mozlang PUBLIC ${llvm_libs})
