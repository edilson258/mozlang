#pragma once

#include <string>

enum class token_type
{
  eof = 1,
  string = 2,
  ident = 3,
  semic = 4,
  comma = 5,
  lparen = 6,
  rparen = 7,
};

class position
{
public:
  size_t line, col, start, end;

  position() = default;
  position(size_t l, size_t c, size_t s, size_t e) : line(l), col(c), start(s), end(e) {};
};

class token
{
public:
  position pos;
  token_type type;
  std::string lexeme;

  token() = default;
  token(position p, token_type t, std::string l) : pos(p), type(t), lexeme(l) {};

  std::string inspect();
};
