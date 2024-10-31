cmake_minimum_required (VERSION 3.8)

project (Mozlang
    VERSION 0.1.0
    LANGUAGES CXX
    DESCRIPTION "A programing language developed by Mozambicans"
)

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