cmake_minimum_required (VERSION 3.8)

project ("Mozlang")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable (Mozlang 
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
)