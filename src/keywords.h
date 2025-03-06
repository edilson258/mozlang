#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

#include "token.h"

const std::unordered_map<std::string, TokenType> KEYWORDS = {
    std::make_pair("fun", TokenType::FUN),
    std::make_pair("return", TokenType::RETURN),

    // Numbers
    std::make_pair("i8", TokenType::I8),
    std::make_pair("i16", TokenType::I16),
    std::make_pair("i32", TokenType::I32),
    std::make_pair("i64", TokenType::I64),
    std::make_pair("u8", TokenType::U8),
    std::make_pair("u16", TokenType::U16),
    std::make_pair("u32", TokenType::U32),
    std::make_pair("u64", TokenType::U64),
    std::make_pair("f8", TokenType::F8),
    std::make_pair("f16", TokenType::F16),
    std::make_pair("f32", TokenType::F32),
    std::make_pair("f64", TokenType::F64),

    std::make_pair("byte", TokenType::BYTE),
    std::make_pair("void", TokenType::VOID),
    std::make_pair("string", TokenType::STRING_T),

    std::make_pair("let", TokenType::LET),
    std::make_pair("import", TokenType::IMPORT),
    std::make_pair("from", TokenType::FROM),
    std::make_pair("pub", TokenType::PUB),
};

class Keyword
{
public:
  static std::optional<TokenType> match(std::string);
};
