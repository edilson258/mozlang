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
    auto stmt_res = parse_statement();
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

result<std::shared_ptr<statement>, error> parser::parse_statement()
{
  auto expr_res = parse_expression(precedence::lowest);
  if (expr_res.is_err())
  {
    return result<std::shared_ptr<statement>, error>(expr_res.unwrap_err());
  }
  auto expect_res = expect(token_type::semic);
  if (expect_res.is_err())
  {
    return result<std::shared_ptr<statement>, error>(expect_res.unwrap_err());
  }
  std::shared_ptr<expression> expr = expr_res.unwrap();
  expr.get()->pos.end = expect_res.unwrap().end;
  return result<std::shared_ptr<statement>, error>(expr);
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

result<std::shared_ptr<expression>, error> parser::parse_expression(precedence prec)
{
  auto lhs_res = parse_expression_lhs();
  if (lhs_res.is_err())
  {
    return result<std::shared_ptr<expression>, error>(lhs_res.unwrap_err());
  }

  next();

  while (!is_eof() && prec < token2prec(tkn.type))
  {
    switch (tkn.type)
    {
    case token_type::lparen:
    {
      auto expr_call_res = parse_expression_call(lhs_res.unwrap());
      if (expr_call_res.is_err())
      {
        return result<std::shared_ptr<expression>, error>(lhs_res.unwrap_err());
      }
      lhs_res.set_val(expr_call_res.unwrap());
    }
    default:
      goto defer;
    }
  }

defer:
  return result<std::shared_ptr<expression>, error>(lhs_res.unwrap());
}

result<std::shared_ptr<expression>, error> parser::parse_expression_lhs()
{
  switch (tkn.type)
  {
  case token_type::string:
    return result<std::shared_ptr<expression>, error>(std::make_shared<expression_string>(tkn.pos, tkn.lexeme));
  case token_type::ident:
    return result<std::shared_ptr<expression>, error>(std::make_shared<expression_identifier>(tkn.pos, tkn.lexeme));
  default:
    std::cout << static_cast<int>(tkn.type) << std::endl;
    assert(false);
  }
}

result<std::shared_ptr<expression_call>, error> parser::parse_expression_call(std::shared_ptr<expression> callee)
{
  auto expect_res = expect(token_type::lparen);
  if (expect_res.is_err())
  {
    return result<std::shared_ptr<expression_call>, error>(expect_res.unwrap_err());
  }

  position pos = callee.get()->pos;
  std::vector<std::shared_ptr<expression>> args;

  for (;;)
  {
    if (token_type::rparen == tkn.type)
    {
      break;
    }

    auto expr_res = parse_expression(precedence::lowest);
    if (expr_res.is_err())
    {
      return result<std::shared_ptr<expression_call>, error>(expr_res.unwrap_err());
    }

    args.push_back(expr_res.unwrap());

    if (token_type::rparen != tkn.type)
    {
      expect_res = expect(token_type::comma);
      if (expect_res.is_err())
      {
        return result<std::shared_ptr<expression_call>, error>(expect_res.unwrap_err());
      }
    }
  }

  expect_res = expect(token_type::rparen);
  if (expect_res.is_err())
  {
    return result<std::shared_ptr<expression_call>, error>(expect_res.unwrap_err());
  }

  pos.end = expect_res.unwrap().end;

  return result<std::shared_ptr<expression_call>, error>(std::make_shared<expression_call>(pos, callee, args));
}

result<position, error> parser::next()
{
  position pos = tkn.pos;
  auto res = lexr.next();
  assert(res.is_ok());
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
