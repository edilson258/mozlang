#include <cassert>
#include <memory>
#include <optional>
#include <vector>

#include "ast.h"
#include "diagnostic.h"
#include "parser.h"
#include "result.h"
#include "token.h"
#include "type.h"

Result<AST, Diagnostic> Parser::Parse()
{
  Next().unwrap();
  Next().unwrap();

  AST ast;
  while (!IsEof())
  {
    auto stmtRes = ParseStatement();
    if (stmtRes.is_ok())
    {
      ast.m_Program.push_back(stmtRes.unwrap());
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
  if (TokenType::FUN == m_CurrToken.m_Type)
  {
    auto result = ParseStatementFunction();
    if (result.is_err())
    {
      return Result<std::shared_ptr<Statement>, Diagnostic>(result.unwrap_err());
    }
    return Result<std::shared_ptr<Statement>, Diagnostic>(result.unwrap());
  }

  if (TokenType::RETURN == m_CurrToken.m_Type)
  {
    auto result = ParseStatementReturn();
    if (result.is_err())
    {
      return Result<std::shared_ptr<Statement>, Diagnostic>(result.unwrap_err());
    }
    return Result<std::shared_ptr<Statement>, Diagnostic>(result.unwrap());
  }

  auto expressionRes = ParseExpression(Precedence::LOWEST);
  if (expressionRes.is_err())
  {
    return Result<std::shared_ptr<Statement>, Diagnostic>(expressionRes.unwrap_err());
  }

  auto expression = expressionRes.unwrap();

  if (TokenType::SEMICOLON != m_CurrToken.m_Type)
  {
    if (TokenType::RBRACE != m_CurrToken.m_Type)
    {
      return Result<std::shared_ptr<Statement>, Diagnostic>(Diagnostic(Errno::SYNTAX_ERROR, expression.get()->m_Position, m_Lexer.m_Source, DiagnosticSeverity::ERROR, "implicity return expression must be the last in a block, insert ';' at end"));
    }
    return Result<std::shared_ptr<Statement>, Diagnostic>(std::make_shared<StatementReturn>(StatementReturn(expression->m_Position, expression)));
  }
  else
  {
    Next().unwrap();
  }

  return Result<std::shared_ptr<Statement>, Diagnostic>(expression);
}

Result<FunctionParams, Diagnostic> Parser::ParseFunctionParams()
{
  auto expectRes = Expect(TokenType::LPAREN);
  assert(expectRes.is_ok());
  Position position = expectRes.unwrap();

  std::vector<FunctionParam> params;

  while (!IsEof())
  {
    if (TokenType::RPAREN == m_CurrToken.m_Type)
    {
      break;
    }

    if (TokenType::IDENTIFIER != m_CurrToken.m_Type)
    {
      return Result<FunctionParams, Diagnostic>(Diagnostic(Errno::SYNTAX_ERROR, m_CurrToken.m_Position, m_Lexer.m_Source, DiagnosticSeverity::ERROR, "expect param name"));
    }
    auto paramIdentifier = std::make_shared<ExpressionIdentifier>(m_CurrToken.m_Position, m_CurrToken.m_Lexeme);
    Position paramPosition = Next().unwrap();
    TypeAnnotationToken typeAnnotation(std::nullopt, std::make_shared<type::Type>(type::Type(type::Base::ANY)));
    if (TokenType::COLON == m_CurrToken.m_Type)
    {
      Expect(TokenType::COLON).unwrap();
      typeAnnotation.m_ReturnType = ParseTypeAnnotation().unwrap();
      typeAnnotation.m_Position = Next().unwrap();
    }
    params.push_back(FunctionParam(paramPosition, paramIdentifier, typeAnnotation));

    if (TokenType::RPAREN != m_CurrToken.m_Type)
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
  position.m_End = expectRes.unwrap().m_End;

  return Result<FunctionParams, Diagnostic>(FunctionParams(position, std::move(params)));
}

Result<std::shared_ptr<StatementFunction>, Diagnostic> Parser::ParseStatementFunction()
{
  Position position = Expect(TokenType::FUN).unwrap();
  if (TokenType::IDENTIFIER != m_CurrToken.m_Type)
  {
    return Result<std::shared_ptr<StatementFunction>, Diagnostic>(Diagnostic(Errno::SYNTAX_ERROR, m_CurrToken.m_Position, m_Lexer.m_Source, DiagnosticSeverity::ERROR, "expect identifier after 'fn'"));
  }
  auto name = std::make_shared<ExpressionIdentifier>(m_CurrToken.m_Position, m_CurrToken.m_Lexeme);
  Next();
  auto paramsRes = ParseFunctionParams();
  TypeAnnotationToken typeAnnotation(std::nullopt, std::make_shared<type::Type>(type::Type(type::Base::ANY)));
  if (TokenType::COLON == m_CurrToken.m_Type)
  {
    Expect(TokenType::COLON).unwrap();
    typeAnnotation.m_ReturnType = ParseTypeAnnotation().unwrap();
    typeAnnotation.m_Position = Next().unwrap();
  }
  if (paramsRes.is_err())
  {
    return Result<std::shared_ptr<StatementFunction>, Diagnostic>(paramsRes.unwrap_err());
  }
  auto bodyRes = ParseStatementBlock();
  assert(bodyRes.is_ok());
  position.m_End = bodyRes.unwrap().get()->m_Position.m_End;
  return Result<std::shared_ptr<StatementFunction>, Diagnostic>(std::make_shared<StatementFunction>(StatementFunction(position, name, paramsRes.unwrap(), bodyRes.unwrap(), typeAnnotation)));
}

Result<std::shared_ptr<StatementBlock>, Diagnostic> Parser::ParseStatementBlock()
{
  Position position = Expect(TokenType::LBRACE).unwrap();
  std::vector<std::shared_ptr<Statement>> statements = {};
  while (!IsEof() && TokenType::RBRACE != m_CurrToken.m_Type)
  {
    auto statementRes = ParseStatement();
    if (statementRes.is_err())
    {
      return Result<std::shared_ptr<StatementBlock>, Diagnostic>(statementRes.unwrap_err());
    }
    statements.push_back(statementRes.unwrap());
  }
  position.m_End = Expect(TokenType::RBRACE).unwrap().m_End;
  return Result<std::shared_ptr<StatementBlock>, Diagnostic>(std::make_shared<StatementBlock>(position, std::move(statements)));
}

Result<std::shared_ptr<StatementReturn>, Diagnostic> Parser::ParseStatementReturn()
{
  Position position = Expect(TokenType::RETURN).unwrap();
  std::optional<std::shared_ptr<Expression>> value = std::nullopt;
  if (TokenType::SEMICOLON != m_CurrToken.m_Type)
  {
    auto valueRes = ParseExpression(Precedence::LOWEST);
    assert(valueRes.is_ok());
    value = valueRes.unwrap();
    position.m_End = value->get()->m_Position.m_End;
  }
  Expect(TokenType::SEMICOLON).unwrap();
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
  auto lhs_res = ParseExpressionLhs();
  if (lhs_res.is_err())
  {
    return Result<std::shared_ptr<Expression>, Diagnostic>(lhs_res.unwrap_err());
  }
  Next(); // eat lhs expression
  while (!IsEof() && prec < token2precedence(m_CurrToken.m_Type))
  {
    switch (m_CurrToken.m_Type)
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

Result<std::shared_ptr<Expression>, Diagnostic> Parser::ParseExpressionLhs()
{
  switch (m_CurrToken.m_Type)
  {
  case TokenType::STRING:
    return Result<std::shared_ptr<Expression>, Diagnostic>(std::make_shared<ExpressionString>(ExpressionString(m_CurrToken.m_Position, m_CurrToken.m_Lexeme)));
  case TokenType::IDENTIFIER:
    return Result<std::shared_ptr<Expression>, Diagnostic>(std::make_shared<ExpressionIdentifier>(ExpressionIdentifier(m_CurrToken.m_Position, m_CurrToken.m_Lexeme)));
  default:
    // TODO: display expression
    return Result<std::shared_ptr<Expression>, Diagnostic>(Diagnostic(Errno::SYNTAX_ERROR, m_CurrToken.m_Position, m_Lexer.m_Source, DiagnosticSeverity::ERROR, "invalid left side expression"));
  }
}

Result<std::shared_ptr<ExpressionCall>, Diagnostic> Parser::ParseExpressionCall(std::shared_ptr<Expression> callee)
{
  auto expect_res = Expect(TokenType::LPAREN);
  if (expect_res.is_err())
  {
    return Result<std::shared_ptr<ExpressionCall>, Diagnostic>(expect_res.unwrap_err());
  }
  Position pos = callee.get()->m_Position;
  Position args_pos = expect_res.unwrap();
  std::vector<std::shared_ptr<Expression>> args;
  for (;;)
  {
    if (TokenType::RPAREN == m_CurrToken.m_Type)
    {
      break;
    }
    auto expr_res = ParseExpression(Precedence::LOWEST);
    if (expr_res.is_err())
    {
      return Result<std::shared_ptr<ExpressionCall>, Diagnostic>(expr_res.unwrap_err());
    }
    args.push_back(expr_res.unwrap());
    if (TokenType::RPAREN != m_CurrToken.m_Type)
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
  pos.m_End = expect_res.unwrap().m_End;
  args_pos.m_End = expect_res.unwrap().m_End;
  return Result<std::shared_ptr<ExpressionCall>, Diagnostic>(std::make_shared<ExpressionCall>(pos, callee, args, args_pos));
}

Result<std::shared_ptr<type::Type>, Diagnostic> Parser::ParseTypeAnnotation()
{
  switch (m_CurrToken.m_Type)
  {
  case TokenType::I32:
    return Result<std::shared_ptr<type::Type>, Diagnostic>(std::make_shared<type::Type>(type::Type(type::Base::I32)));
  case TokenType::STRING_T:
    return Result<std::shared_ptr<type::Type>, Diagnostic>(std::make_shared<type::Type>(type::Type(type::Base::STRING)));
  default:
    return Result<std::shared_ptr<type::Type>, Diagnostic>(Diagnostic(Errno::SYNTAX_ERROR, m_CurrToken.m_Position, m_Lexer.m_Source, DiagnosticSeverity::ERROR, "expect type annotation, try 'i32', 'string', ..."));
  }
}

Result<Position, Diagnostic> Parser::Next()
{
  Position pos = m_CurrToken.m_Position;
  auto res = m_Lexer.Next();
  if (res.is_err())
  {
    return Result<Position, Diagnostic>(res.unwrap_err());
  }
  m_CurrToken = m_NextToken;
  m_NextToken = res.unwrap();
  return Result<Position, Diagnostic>(pos);
}

Result<Position, Diagnostic> Parser::Expect(TokenType tokenType)
{
  if (tokenType != m_CurrToken.m_Type)
  {
    // TODO: display token, token_type
    return Result<Position, Diagnostic>(Diagnostic(Errno::SYNTAX_ERROR, m_CurrToken.m_Position, m_Lexer.m_Source, DiagnosticSeverity::ERROR, "syntax error: expect {} but got {}."));
  }
  return Next();
}

bool Parser::IsEof()
{
  return TokenType::EOf == m_CurrToken.m_Type;
}
