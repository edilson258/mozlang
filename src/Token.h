#pragma once

#include <optional>
#include <string>

enum class TokenType
{
  Eof,

  // Keywords
  Fn,
  Return,

  TypeInt,
  TypeStr,
  TypeVoid,

  // Literals
  String,
  Integer,

  // Symbols
  Identifier,

  // Punct.
  LeftParent,
  RightParent,
  LeftBrace,
  RightBrace,
  Semicolon,
  Colon,
  Comma,
};

class TokenSpan
{
public:
  unsigned long Line;
  unsigned long Column;
  unsigned long RangeBegin;
  unsigned long RangeEnd;

  TokenSpan() = default;
  TokenSpan(unsigned long line, unsigned long column, unsigned long begin, unsigned long end)
      : Line(line), Column(column), RangeBegin(begin), RangeEnd(end)
  {
  }
};

class Token
{
public:
  TokenType Type;
  TokenSpan Span;
  std::optional<std::string> Data;

  std::string ToString();

  Token() = default;
  Token(TokenType type, TokenSpan span) : Type(type), Span(span), Data(std::nullopt) {};
  Token(TokenType type, TokenSpan span, std::string data) : Type(type), Span(span), Data(data) {};
};
