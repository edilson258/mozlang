#include <cassert>
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <string>

#include "lexer.h"
#include "result.h"
#include "token.h"

#define EOF_CHAR '\0'

result<token, error> lexer::next()
{
  advance_while([](char c)
                { return std::isspace(c); });

  if (is_eof())
  {
    return result<token, error>(token(position(line, col, cursor, cursor), token_type::eof, "EOF"));
  }

  char current = peek_one();
  switch (current)
  {
  case '(':
    return make_token_simple(token_type::lparen);
  case ')':
    return make_token_simple(token_type::rparen);
  case ';':
    return make_token_simple(token_type::semic);
  case ',':
    return make_token_simple(token_type::comma);
  case '"':
    return make_token_string();
  }

  if (std::isalpha(current) || '_' == current)
  {
    size_t at = cursor;
    size_t at_col = col;
    size_t len = advance_while([](char c)
                               { return std::isalnum(c) || '_' == c; });
    return result<token, error>(token(position(line, at_col, at, cursor - 1), token_type::ident, src.get()->content.substr(at, len)));
  }

  assert(false);
}

result<token, error> lexer::make_token_simple(token_type tt)
{
  token tkn(position(line, col, cursor, cursor), tt, std::string(1, peek_one()));
  advance();
  return result<token, error>(tkn);
}

result<token, error> lexer::make_token_string()
{
  size_t at = cursor;
  size_t at_col = col;

  advance();
  for (;;)
  {
    char current = peek_one();
    if (is_eof() || '\n' == current)
    {
      assert(false);
    }
    if ('"' == current)
    {
      advance();
      break;
    }
    advance();
  }

  return result<token, error>(token(position(line, at_col, at, cursor - 1), token_type::string, src.get()->content.substr(at, cursor - at)));
}

bool lexer::is_eof()
{
  return cursor >= src.get()->content.length();
}

char lexer::peek_one()
{
  if (is_eof())
  {
    return EOF_CHAR;
  }
  return src.get()->content.at(cursor);
}

void lexer::advance()
{
  if (is_eof())
  {
    return;
  }
  cursor++;
  if ('\n' == peek_one())
  {
    line++;
    col = 0;
  }
  else
  {
    col++;
  }
}

size_t lexer::advance_while(std::function<bool(char)> pred)
{
  size_t at = cursor;
  while (!is_eof() && pred(peek_one()))
  {
    advance();
  }
  return cursor - at;
}
