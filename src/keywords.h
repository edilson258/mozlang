#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

#include "token.h"

const std::unordered_map<std::string, TokenType> KEYWORDS = {
    std::make_pair("fun", TokenType::FUN),
    std::make_pair("return", TokenType::RETURN),
    // std::make_pair(name, type),
};

class Keyword
{
public:
  static std::optional<TokenType> match(std::string);
};
