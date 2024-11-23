#pragma once

#include "Location.h"

#include <string>

enum class TokenType
{
  Eof,

  // Keywords
  Fn,
  Return,

  True,
  False,

  TypeInt,
  TypeStr,
  TypeVoid,
  TypeBool,

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
  Location Loc;
  std::string Data;

  std::string ToString();

  Token() = default;
  Token(TokenType type, Location loc, std::string data) : Type(type), Loc(loc), Data(data) {};
};
