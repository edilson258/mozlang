#pragma once

#include "ast.h"
#include "lexer.h"

class parser
{
public:
  parser(lexer l) : lexr(l), tkn() {};

  result<std::shared_ptr<ast>, error> parse();

private:
  lexer lexr;
  token tkn;

  bool is_eof();
  result<position, error> next();
  result<position, error> expect(token_type);

  result<std::shared_ptr<stmt>, error> parse_stmt();
  result<std::shared_ptr<expr>, error> parse_expr(precedence);
  result<std::shared_ptr<expr>, error> parse_expr_lhs();
  result<std::shared_ptr<expr_call>, error> parse_expr_call(std::shared_ptr<expr>);
};
