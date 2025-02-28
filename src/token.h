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

  // Types
  I32,
  STRING_T,

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
  size_t m_Line, m_Column, m_Start, m_End;

  Position() = default;
  Position(size_t line, size_t column, size_t start, size_t end) : m_Line(line), m_Column(column), m_Start(start), m_End(end) {};
};

class Token
{
public:
  Position m_Position;
  TokenType m_Type;
  std::string m_Lexeme;

  Token() = default;
  Token(Position position, TokenType type, std::string lexeme) : m_Position(position), m_Type(type), m_Lexeme(lexeme) {};

  std::string Inspect();
};
