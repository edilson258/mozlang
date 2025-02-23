#include <cassert>
#include <iostream>

#include "ast.h"
#include "parser.h"
#include "token.h"

result<std::shared_ptr<ast>, error> parser::parse()
{
  auto res = next();
  if (res.is_err())
  {
    return result<std::shared_ptr<ast>, error>(res.unwrap_err());
  }
  auto tree = std::make_shared<ast>();
  while (!is_eof())
  {
    auto stmt_res = parse_stmt();
    if (stmt_res.is_ok())
    {
      tree.get()->program.push_back(stmt_res.unwrap());
    }
    else
    {
      return result<std::shared_ptr<ast>, error>(stmt_res.unwrap_err());
    }
  }
  return result<std::shared_ptr<ast>, error>(tree);
}

result<std::shared_ptr<stmt>, error> parser::parse_stmt()
{
  auto expr_res = parse_expr(precedence::lowest);
  if (expr_res.is_err())
  {
    return result<std::shared_ptr<stmt>, error>(expr_res.unwrap_err());
  }
  auto expect_res = expect(token_type::semic);
  if (expect_res.is_err())
  {
    return result<std::shared_ptr<stmt>, error>(expect_res.unwrap_err());
  }
  std::shared_ptr<expr> exp = expr_res.unwrap();
  exp.get()->pos.end = expect_res.unwrap().end;
  return result<std::shared_ptr<stmt>, error>(exp);
}

precedence token2prec(token_type tt)
{
  switch (tt)
  {
  case token_type::lparen:
    return precedence::call;
  default:
    return precedence::lowest;
  }
}

result<std::shared_ptr<expr>, error> parser::parse_expr(precedence prec)
{
  auto lhs_res = parse_expr_lhs();
  if (lhs_res.is_err())
  {
    return result<std::shared_ptr<expr>, error>(lhs_res.unwrap_err());
  }
  next();
  while (!is_eof() && prec < token2prec(tkn.type))
  {
    switch (tkn.type)
    {
    case token_type::lparen:
    {
      auto expr_call_res = parse_expr_call(lhs_res.unwrap());
      if (expr_call_res.is_err())
      {
        return result<std::shared_ptr<expr>, error>(lhs_res.unwrap_err());
      }
      lhs_res.set_val(expr_call_res.unwrap());
    }
    default:
      goto defer;
    }
  }
defer:
  return result<std::shared_ptr<expr>, error>(lhs_res.unwrap());
}

result<std::shared_ptr<expr>, error> parser::parse_expr_lhs()
{
  switch (tkn.type)
  {
  case token_type::string:
    return result<std::shared_ptr<expr>, error>(std::make_shared<expr_string>(tkn.pos, tkn.lexeme));
  case token_type::ident:
    return result<std::shared_ptr<expr>, error>(std::make_shared<expr_ident>(tkn.pos, tkn.lexeme));
  default:
    std::cout << static_cast<int>(tkn.type) << std::endl;
    assert(false);
  }
}

result<std::shared_ptr<expr_call>, error> parser::parse_expr_call(std::shared_ptr<expr> callee)
{
  auto expect_res = expect(token_type::lparen);
  if (expect_res.is_err())
  {
    return result<std::shared_ptr<expr_call>, error>(expect_res.unwrap_err());
  }
  position pos = callee.get()->pos;
  std::vector<std::shared_ptr<expr>> args;
  for (;;)
  {
    if (token_type::rparen == tkn.type)
    {
      break;
    }
    auto expr_res = parse_expr(precedence::lowest);
    if (expr_res.is_err())
    {
      return result<std::shared_ptr<expr_call>, error>(expr_res.unwrap_err());
    }
    args.push_back(expr_res.unwrap());
    if (token_type::rparen != tkn.type)
    {
      expect_res = expect(token_type::comma);
      if (expect_res.is_err())
      {
        return result<std::shared_ptr<expr_call>, error>(expect_res.unwrap_err());
      }
    }
  }
  expect_res = expect(token_type::rparen);
  if (expect_res.is_err())
  {
    return result<std::shared_ptr<expr_call>, error>(expect_res.unwrap_err());
  }
  pos.end = expect_res.unwrap().end;
  return result<std::shared_ptr<expr_call>, error>(std::make_shared<expr_call>(pos, callee, args));
}

result<position, error> parser::next()
{
  position pos = tkn.pos;
  auto res = lexr.next();
  if (res.is_err())
  {
    return result<position, error>(res.unwrap_err());
  }
  tkn = res.unwrap();
  return result<position, error>(pos);
}

result<position, error> parser::expect(token_type tt)
{
  if (tt != tkn.type)
  {
    std::cout << static_cast<int>(tt) << std::endl;
    assert(false);
  }
  return next();
}

bool parser::is_eof()
{
  return token_type::eof == tkn.type;
}
