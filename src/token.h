#pragma once

#include <string>

enum class TokenType
{
  Ident,

  StrLit,
  BinLit,
  HexLit,
  DecLit,
  FloatLit,

  // Keywords
  Import,
  Fun,
  Ret,
  Let,
  From,
  Pub,
  Class,

  I8,
  I16,
  I32,
  I64,
  U8,
  U16,
  U32,
  U64,
  Float,
  Void,
  String,

  At, // @
  Comma,
  Colon,
  Assoc, // ::
  Lparen,
  Rparen,
  Lbrace,
  Rbrace,
  Equal,
  Semi,
  Dot,
  Arrow,    // ->
  Ellipsis, // ...

  Plus,     // +
  Minus,    // -
  Asterisk, // *
  Slash,    // /

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
