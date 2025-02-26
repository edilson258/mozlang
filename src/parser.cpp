#include <iostream>
#include <memory>

#include "ast.h"
#include "parser.h"
#include "token.h"

result<AST, Diagnostic> parser::parse()
{
  auto res = next();
  if (res.is_err())
  {
    return result<AST, Diagnostic>(res.unwrap_err());
  }
  AST tree;
  while (!is_eof())
  {
    auto stmt_res = parse_stmt();
    if (stmt_res.is_ok())
    {
      tree.Program.push_back(stmt_res.unwrap());
    }
    else
    {
      return result<AST, Diagnostic>(stmt_res.unwrap_err());
    }
  }
  return result<AST, Diagnostic>(tree);
}

result<std::shared_ptr<Statement>, Diagnostic> parser::parse_stmt()
{
  auto expr_res = parse_expr(Precedence::LOWEST);
  if (expr_res.is_err())
  {
    return result<std::shared_ptr<Statement>, Diagnostic>(expr_res.unwrap_err());
  }
  auto expect_res = expect(TokenType::SEMICOLON);
  if (expect_res.is_err())
  {
    return result<std::shared_ptr<Statement>, Diagnostic>(expect_res.unwrap_err());
  }
  std::shared_ptr<Expression> exp = expr_res.unwrap();
  exp.get()->Pos.End = expect_res.unwrap().End;
  return result<std::shared_ptr<Statement>, Diagnostic>(exp);
}

Precedence token2precedence(TokenType tt)
{
  switch (tt)
  {
  case TokenType::LPAREN:
    return Precedence::CALL;
  default:
    return Precedence::LOWEST;
  }
}

result<std::shared_ptr<Expression>, Diagnostic> parser::parse_expr(Precedence prec)
{
  auto lhs_res = parse_expr_lhs();
  if (lhs_res.is_err())
  {
    return result<std::shared_ptr<Expression>, Diagnostic>(lhs_res.unwrap_err());
  }
  next(); // eat lhs expression
  while (!is_eof() && prec < token2precedence(tkn.Type))
  {
    switch (tkn.Type)
    {
    case TokenType::LPAREN:
    {
      auto expr_call_res = parse_expr_call(lhs_res.unwrap());
      if (expr_call_res.is_err())
      {
        return result<std::shared_ptr<Expression>, Diagnostic>(lhs_res.unwrap_err());
      }
      lhs_res.set_val(expr_call_res.unwrap());
    }
    default:
      goto defer;
    }
  }
defer:
  return result<std::shared_ptr<Expression>, Diagnostic>(lhs_res.unwrap());
}

result<std::shared_ptr<Expression>, Diagnostic> parser::parse_expr_lhs()
{
  switch (tkn.Type)
  {
  case TokenType::STRING:
    return result<std::shared_ptr<Expression>, Diagnostic>(std::make_shared<ExpressionString>(tkn.Pos, tkn.Lexeme));
  case TokenType::IDENTIFIER:
    return result<std::shared_ptr<Expression>, Diagnostic>(std::make_shared<ExpressionIdentifier>(tkn.Pos, tkn.Lexeme));
  default:
    // TODO: display expression
    return result<std::shared_ptr<Expression>, Diagnostic>(Diagnostic(Errno::SYNTAX_ERROR, tkn.Pos, lexr.Sourc, DiagnosticSeverity::ERROR, "invalid left side expression"));
  }
}

result<std::shared_ptr<ExpressionCall>, Diagnostic> parser::parse_expr_call(std::shared_ptr<Expression> callee)
{
  auto expect_res = expect(TokenType::LPAREN);
  if (expect_res.is_err())
  {
    return result<std::shared_ptr<ExpressionCall>, Diagnostic>(expect_res.unwrap_err());
  }
  Position pos = callee.get()->Pos;
  Position args_pos = expect_res.unwrap();
  std::vector<std::shared_ptr<Expression>> args;
  for (;;)
  {
    if (TokenType::RPAREN == tkn.Type)
    {
      break;
    }
    auto expr_res = parse_expr(Precedence::LOWEST);
    if (expr_res.is_err())
    {
      return result<std::shared_ptr<ExpressionCall>, Diagnostic>(expr_res.unwrap_err());
    }
    args.push_back(expr_res.unwrap());
    if (TokenType::RPAREN != tkn.Type)
    {
      expect_res = expect(TokenType::COMMA);
      if (expect_res.is_err())
      {
        return result<std::shared_ptr<ExpressionCall>, Diagnostic>(expect_res.unwrap_err());
      }
    }
  }
  expect_res = expect(TokenType::RPAREN);
  if (expect_res.is_err())
  {
    return result<std::shared_ptr<ExpressionCall>, Diagnostic>(expect_res.unwrap_err());
  }
  pos.End = expect_res.unwrap().End;
  args_pos.End = expect_res.unwrap().End;
  return result<std::shared_ptr<ExpressionCall>, Diagnostic>(std::make_shared<ExpressionCall>(pos, callee, args, args_pos));
}

result<Position, Diagnostic> parser::next()
{
  Position pos = tkn.Pos;
  auto res = lexr.Next();
  if (res.is_err())
  {
    return result<Position, Diagnostic>(res.unwrap_err());
  }
  tkn = res.unwrap();
  return result<Position, Diagnostic>(pos);
}

result<Position, Diagnostic> parser::expect(TokenType tt)
{
  if (tt != tkn.Type)
  {
    std::cout << static_cast<int>(tt) << std::endl;
    // TODO: display token, token_type
    return result<Position, Diagnostic>(Diagnostic(Errno::SYNTAX_ERROR, tkn.Pos, lexr.Sourc, DiagnosticSeverity::ERROR, "syntax error: expect {} but got {}."));
  }
  return next();
}

bool parser::is_eof()
{
  return TokenType::EOf == tkn.Type;
}
