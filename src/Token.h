#pragma once

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

  bool operator==(const TokenSpan &other) const
  {
    if (Line == other.Line && Column == other.Column && RangeBegin == other.RangeBegin && RangeEnd == other.RangeEnd)
    {
      return true;
    }
    return false;
  }
};

class Token
{
public:
  TokenType Type;
  TokenSpan Span;
  std::string Data;

  std::string ToString();

  Token() = default;
  Token(TokenType type, TokenSpan span, std::string data) : Type(type), Span(span), Data(data) {};
};
