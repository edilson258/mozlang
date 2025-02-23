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

  result<std::shared_ptr<statement>, error> parse_statement();
  result<std::shared_ptr<expression>, error> parse_expression(precedence);
  result<std::shared_ptr<expression>, error> parse_expression_lhs();
  result<std::shared_ptr<expression_call>, error> parse_expression_call(std::shared_ptr<expression>);
};
