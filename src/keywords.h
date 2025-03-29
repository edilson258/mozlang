#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

#include "token.h"

const std::unordered_map<std::string, TokenType> KEYWORDS = {
    std::make_pair("import", TokenType::Import),
    std::make_pair("fun", TokenType::Fun),
    std::make_pair("return", TokenType::Ret),
    std::make_pair("let", TokenType::Let),
    std::make_pair("from", TokenType::From),
    std::make_pair("pub", TokenType::Pub),
    std::make_pair("class", TokenType::Class),

    std::make_pair("i8", TokenType::I8),
    std::make_pair("i16", TokenType::I16),
    std::make_pair("i32", TokenType::I32),
    std::make_pair("i64", TokenType::I64),
    std::make_pair("u8", TokenType::U8),
    std::make_pair("u16", TokenType::U16),
    std::make_pair("u32", TokenType::U32),
    std::make_pair("u64", TokenType::U64),
    std::make_pair("float", TokenType::Float),

    std::make_pair("void", TokenType::Void),
    std::make_pair("string", TokenType::String),
};

class Keyword
{
public:
  static std::optional<TokenType> match(std::string);
};
