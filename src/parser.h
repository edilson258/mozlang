#pragma once

#include "ast.h"
#include "diagnostic.h"
#include "lexer.h"

class parser
{
public:
  parser(Lexer l) : lexr(l), tkn() {};

  result<AST, Diagnostic> parse();

private:
  Lexer lexr;
  Token tkn;

  bool is_eof();
  result<Position, Diagnostic> next();
  result<Position, Diagnostic> expect(TokenType);

  result<std::shared_ptr<Statement>, Diagnostic> parse_stmt();
  result<std::shared_ptr<Expression>, Diagnostic> parse_expr(Precedence);
  result<std::shared_ptr<Expression>, Diagnostic> parse_expr_lhs();
  result<std::shared_ptr<ExpressionCall>, Diagnostic> parse_expr_call(std::shared_ptr<Expression>);
};
