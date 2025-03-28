#pragma once

#include <string>

enum class TokenType
{
  IDENT,

  STRING,
  NUMBER,

  // Keywords
  IMPORT,
  FUN,
  RETURN,
  LET,
  FROM,
  PUB,
  CLASS,

  I8,
  I16,
  I32,
  I64,
  U8,
  U16,
  U32,
  U64,
  F8,
  F16,
  F32,
  F64,
  VOID,
  STR_TYP,

  AT, // @
  COMMA,
  COLON,
  ASSOC, // ::
  LPAREN,
  RPAREN,
  LBRACE,
  RBRACE,
  EQUAL,
  SEMICOLON,
  DOT,
  ARROW,    // ->
  ELLIPSIS, // ...

  PLUS,     // +
  MINUS,    // -
  ASTERISK, // *
  SLASH,    // /

  END,
};

class Position
{
public:
  size_t m_Line, m_Column, m_Start, m_End;

  Position() = default;
  Position(size_t line, size_t column, size_t start, size_t end) : m_Line(line), m_Column(column), m_Start(start), m_End(end) {};

  Position MergeWith(const Position &other) const
  {
    return Position(m_Line, m_Column, m_Start, other.m_End);
  }
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
