#pragma once

#include <string>

enum class TokenType
{
  EOf = 1,

  STRING,
  IDENTIFIER,

  // Keywords
  FUN,
  RETURN,

  // Punctuation
  COMMA,
  COLON,
  LPAREN,
  RPAREN,
  LBRACE,
  RBRACE,
  SEMICOLON,

  PLUS,
};

class Position
{
public:
  size_t Line, Column, Start, End;

  Position() = default;
  Position(size_t line, size_t column, size_t start, size_t end) : Line(line), Column(column), Start(start), End(end) {};
};

class Token
{
public:
  Position Pos;
  TokenType Type;
  std::string Lexeme;

  Token() = default;
  Token(Position position, TokenType type, std::string lexeme) : Pos(position), Type(type), Lexeme(lexeme) {};

  std::string Inspect();
};
