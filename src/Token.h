#pragma once

#include "Span.h"

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

class Token
{
public:
  TokenType Type;
  Span Spn;
  std::string Data;

  std::string ToString();

  Token() = default;
  Token(TokenType type, Span span, std::string data) : Type(type), Spn(span), Data(data) {};
};
