#pragma once

#include <cstddef>
#include <functional>
#include <memory>

#include "error.h"
#include "loader.h"
#include "result.h"
#include "token.h"

class lexer
{
public:
  lexer(std::shared_ptr<source> s) : src(s), line(1), col(1), cursor(0) {};

  result<token, error> next();

private:
  std::shared_ptr<source> src;

  size_t line;
  size_t col;

  size_t cursor;

  bool is_eof();
  char peek_one();
  void advance();
  size_t advance_while(std::function<bool(char)>);

  result<token, error> make_token_string();
  result<token, error> make_token_simple(token_type);
};
