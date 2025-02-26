#include <cassert>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ast.h"
#include "diagnostic.h"
#include "parser.h"
#include "result.h"
#include "token.h"

Result<AST, Diagnostic> Parser::Parse()
{
  auto res = Next();
  if (res.is_err())
  {
    return Result<AST, Diagnostic>(res.unwrap_err());
  }
  AST ast;
  while (!IsEof())
  {
    auto stmtRes = ParseStatement();
    if (stmtRes.is_ok())
    {
      ast.Program.push_back(stmtRes.unwrap());
    }
    else
    {
      return Result<AST, Diagnostic>(stmtRes.unwrap_err());
    }
  }
  return Result<AST, Diagnostic>(ast);
}

Result<std::shared_ptr<Statement>, Diagnostic> Parser::ParseStatement()
{
  if (TokenType::FUN == Tokn.Type)
  {
    auto result = ParseStatementFunction();
    if (result.is_err())
    {
      return Result<std::shared_ptr<Statement>, Diagnostic>(result.unwrap_err());
    }
    return Result<std::shared_ptr<Statement>, Diagnostic>(result.unwrap());
  }

  if (TokenType::RETURN == Tokn.Type)
  {
    auto result = ParseStatementReturn();
    if (result.is_err())
    {
      return Result<std::shared_ptr<Statement>, Diagnostic>(result.unwrap_err());
    }
    return Result<std::shared_ptr<Statement>, Diagnostic>(result.unwrap());
  }

  auto expr_res = ParseExpression(Precedence::LOWEST);
  if (expr_res.is_err())
  {
    return Result<std::shared_ptr<Statement>, Diagnostic>(expr_res.unwrap_err());
  }
  auto expect_res = Expect(TokenType::SEMICOLON);
  if (expect_res.is_err())
  {
    return Result<std::shared_ptr<Statement>, Diagnostic>(expect_res.unwrap_err());
  }
  std::shared_ptr<Expression> exp = expr_res.unwrap();
  exp.get()->Pos.End = expect_res.unwrap().End;
  return Result<std::shared_ptr<Statement>, Diagnostic>(exp);
}

Result<FunctionParams, Diagnostic> Parser::ParseFunctionParams()
{
  auto expectRes = Expect(TokenType::LPAREN);
  assert(expectRes.is_ok());
  Position position = expectRes.unwrap();

  std::vector<FunctionParam> params;

  while (!IsEof())
  {
    if (TokenType::RPAREN == Tokn.Type)
    {
      break;
    }

    if (TokenType::IDENTIFIER != Tokn.Type)
    {
      return Result<FunctionParams, Diagnostic>(Diagnostic(Errno::SYNTAX_ERROR, Tokn.Pos, Lexr.Sourc, DiagnosticSeverity::ERROR, "expect param name"));
    }
    auto name = std::make_shared<ExpressionIdentifier>(Tokn.Pos, Tokn.Lexeme);
    Position paramPosition = Next().unwrap();
    if (TokenType::COLON == Tokn.Type)
    {
      assert(false);
    }
    params.push_back(FunctionParam(paramPosition, name, std::nullopt));

    if (TokenType::RPAREN != Tokn.Type)
    {
      expectRes = Expect(TokenType::COMMA);
      if (expectRes.is_err())
      {
        return Result<FunctionParams, Diagnostic>(expectRes.unwrap_err());
      }
    }
  }

  expectRes = Expect(TokenType::RPAREN);
  assert(expectRes.is_ok());
  position.End = expectRes.unwrap().End;

  return Result<FunctionParams, Diagnostic>(FunctionParams(position, std::move(params)));
}

Result<std::shared_ptr<StatementFunction>, Diagnostic> Parser::ParseStatementFunction()
{
  Position position = Expect(TokenType::FUN).unwrap();
  if (TokenType::IDENTIFIER != Tokn.Type)
  {
    return Result<std::shared_ptr<StatementFunction>, Diagnostic>(Diagnostic(Errno::SYNTAX_ERROR, Tokn.Pos, Lexr.Sourc, DiagnosticSeverity::ERROR, "expect identifier after 'fn'"));
  }
  auto name = std::make_shared<ExpressionIdentifier>(Tokn.Pos, Tokn.Lexeme);
  Next();
  auto paramsRes = ParseFunctionParams();
  if (paramsRes.is_err())
  {
    return Result<std::shared_ptr<StatementFunction>, Diagnostic>(paramsRes.unwrap_err());
  }
  auto bodyRes = ParseStatementBlock();
  assert(bodyRes.is_ok());
  position.End = bodyRes.unwrap().get()->Pos.End;
  return Result<std::shared_ptr<StatementFunction>, Diagnostic>(std::make_shared<StatementFunction>(position, name, paramsRes.unwrap(), bodyRes.unwrap()));
}

Result<std::shared_ptr<StatementBlock>, Diagnostic> Parser::ParseStatementBlock()
{
  Position position = Expect(TokenType::LBRACE).unwrap();
  std::vector<std::shared_ptr<Statement>> statements = {};
  while (!IsEof() && TokenType::RBRACE != Tokn.Type)
  {
    auto statementRes = ParseStatement();
    if (statementRes.is_err())
    {
      return Result<std::shared_ptr<StatementBlock>, Diagnostic>(statementRes.unwrap_err());
    }
    statements.push_back(statementRes.unwrap());
  }
  position.End = Expect(TokenType::RBRACE).unwrap().End;
  return Result<std::shared_ptr<StatementBlock>, Diagnostic>(std::make_shared<StatementBlock>(position, std::move(statements)));
}

Result<std::shared_ptr<StatementReturn>, Diagnostic> Parser::ParseStatementReturn()
{
  Position position = Expect(TokenType::RETURN).unwrap();
  std::optional<std::shared_ptr<Expression>> value = std::nullopt;
  if (TokenType::SEMICOLON != Tokn.Type)
  {
    auto valueRes = ParseExpression(Precedence::LOWEST);
    assert(valueRes.is_ok());
    value = valueRes.unwrap();
  }
  position.End = Expect(TokenType::SEMICOLON).unwrap().End;
  return Result<std::shared_ptr<StatementReturn>, Diagnostic>(std::make_shared<StatementReturn>(position, value));
}

Precedence token2precedence(TokenType tokenType)
{
  switch (tokenType)
  {
  case TokenType::LPAREN:
    return Precedence::CALL;
  default:
    return Precedence::LOWEST;
  }
}

Result<std::shared_ptr<Expression>, Diagnostic> Parser::ParseExpression(Precedence prec)
{
  auto lhs_res = ParseExpressionLeft();
  if (lhs_res.is_err())
  {
    return Result<std::shared_ptr<Expression>, Diagnostic>(lhs_res.unwrap_err());
  }
  Next(); // eat lhs expression
  while (!IsEof() && prec < token2precedence(Tokn.Type))
  {
    switch (Tokn.Type)
    {
    case TokenType::LPAREN:
    {
      auto expr_call_res = ParseExpressionCall(lhs_res.unwrap());
      if (expr_call_res.is_err())
      {
        return Result<std::shared_ptr<Expression>, Diagnostic>(lhs_res.unwrap_err());
      }
      lhs_res.set_val(expr_call_res.unwrap());
    }
    default:
      goto defer;
    }
  }
defer:
  return Result<std::shared_ptr<Expression>, Diagnostic>(lhs_res.unwrap());
}

Result<std::shared_ptr<Expression>, Diagnostic> Parser::ParseExpressionLeft()
{
  switch (Tokn.Type)
  {
  case TokenType::STRING:
    return Result<std::shared_ptr<Expression>, Diagnostic>(std::make_shared<ExpressionString>(Tokn.Pos, Tokn.Lexeme));
  case TokenType::IDENTIFIER:
    return Result<std::shared_ptr<Expression>, Diagnostic>(std::make_shared<ExpressionIdentifier>(Tokn.Pos, Tokn.Lexeme));
  default:
    // TODO: display expression
    return Result<std::shared_ptr<Expression>, Diagnostic>(Diagnostic(Errno::SYNTAX_ERROR, Tokn.Pos, Lexr.Sourc, DiagnosticSeverity::ERROR, "invalid left side expression"));
  }
}

Result<std::shared_ptr<ExpressionCall>, Diagnostic> Parser::ParseExpressionCall(std::shared_ptr<Expression> callee)
{
  auto expect_res = Expect(TokenType::LPAREN);
  if (expect_res.is_err())
  {
    return Result<std::shared_ptr<ExpressionCall>, Diagnostic>(expect_res.unwrap_err());
  }
  Position pos = callee.get()->Pos;
  Position args_pos = expect_res.unwrap();
  std::vector<std::shared_ptr<Expression>> args;
  for (;;)
  {
    if (TokenType::RPAREN == Tokn.Type)
    {
      break;
    }
    auto expr_res = ParseExpression(Precedence::LOWEST);
    if (expr_res.is_err())
    {
      return Result<std::shared_ptr<ExpressionCall>, Diagnostic>(expr_res.unwrap_err());
    }
    args.push_back(expr_res.unwrap());
    if (TokenType::RPAREN != Tokn.Type)
    {
      expect_res = Expect(TokenType::COMMA);
      if (expect_res.is_err())
      {
        return Result<std::shared_ptr<ExpressionCall>, Diagnostic>(expect_res.unwrap_err());
      }
    }
  }
  expect_res = Expect(TokenType::RPAREN);
  if (expect_res.is_err())
  {
    return Result<std::shared_ptr<ExpressionCall>, Diagnostic>(expect_res.unwrap_err());
  }
  pos.End = expect_res.unwrap().End;
  args_pos.End = expect_res.unwrap().End;
  return Result<std::shared_ptr<ExpressionCall>, Diagnostic>(std::make_shared<ExpressionCall>(pos, callee, args, args_pos));
}

Result<Position, Diagnostic> Parser::Next()
{
  Position pos = Tokn.Pos;
  auto res = Lexr.Next();
  if (res.is_err())
  {
    return Result<Position, Diagnostic>(res.unwrap_err());
  }
  Tokn = res.unwrap();
  return Result<Position, Diagnostic>(pos);
}

Result<Position, Diagnostic> Parser::Expect(TokenType tt)
{
  if (tt != Tokn.Type)
  {
    std::cout << static_cast<int>(tt) << std::endl;
    // TODO: display token, token_type
    return Result<Position, Diagnostic>(Diagnostic(Errno::SYNTAX_ERROR, Tokn.Pos, Lexr.Sourc, DiagnosticSeverity::ERROR, "syntax error: expect {} but got {}."));
  }
  return Next();
}

bool Parser::IsEof()
{
  return TokenType::EOf == Tokn.Type;
}
