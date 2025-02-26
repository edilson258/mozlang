#include <format>

#include "token.h"

std::string Token::Inspect()
{
  return std::format("{} {}:{}:{}:{}", Lexeme, Pos.Line, Pos.Column, Pos.Start, Pos.End);
}
