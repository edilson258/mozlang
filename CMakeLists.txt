cmake_minimum_required (VERSION 3.8)

project (Mozlang
    VERSION 0.1.0
    LANGUAGES C CXX
    DESCRIPTION "A programing language developed by Mozambicans"
)

set(CMAKE_CXX_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION} at ${LLVM_INSTALL_PREFIX}")
list(APPEND CMAKE_MODULE_PATH ${LLVM_CMAKE_DIR})
include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})
include(AddLLVM)
add_definitions(${LLVM_DEFINITIONS})

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

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

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
else()
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
endif()

add_executable (mozlang 
    "src/Mozlang.cpp"
    "src/Lexer.cpp"
    "src/Lexer.h"
    "src/Token.h"
    "src/Token.cpp"
    "src/Parser.h"
    "src/Parser.cpp"
    "src/Ast.h"
    "src/AstVisitor.h"
    "src/Ast.cpp"
    "src/AstVisitor.cpp"
    "src/CodeGen.h"
    "src/CodeGen.cpp"
)

set(LLVM_LINK_COMPONENTS Core Support nativecodegen)
llvm_map_components_to_libnames(llvmLibs ${LLVM_LINK_COMPONENTS})
target_link_libraries(mozlang PUBLIC ${llvmLibs})
