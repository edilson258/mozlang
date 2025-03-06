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
  LET,
  IMPORT,
  FROM,

  // Types
  VOID,
  STRING_T,
  BYTE,
  // numbers
  // signed integers
  I8,
  I16,
  I32,
  I64,
  // unsigned integers
  U8,
  U16,
  U32,
  U64,
  // floating numbers
  F8,
  F16,
  F32,
  F64,

  // Punctuation
  COMMA,
  COLON,
  LPAREN,
  RPAREN,
  LBRACE,
  RBRACE,
  EQUAL,
  SEMICOLON,
  DOT,
  ARROW, // ->

  // arithmetic
  PLUS,     // +
  MINUS,    // -
  ASTERISK, // *
  SLASH,    // /
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
