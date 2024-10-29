# CMakeList.txt : CMake project for Mozlang, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("Mozlang")

set(CMAKE_CXX_STANDARD 20)           # Replace '17' with your desired standard
set(CMAKE_CXX_STANDARD_REQUIRED ON)   # Make the standard required

# Add source to this project's executable.
add_executable (Mozlang "src/Mozlang.cpp"  "src/Lexer.cpp" "src/Lexer.h" "src/Token.h" "src/Token.cpp" "src/Parser.h" "src/Parser.cpp" "src/Ast.h" "src/AstVisitor.h" "src/Ast.cpp" "src/AstVisitor.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Mozlang PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
