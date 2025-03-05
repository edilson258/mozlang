#include <cassert>
#include <cmath>
#include <memory>
#include <optional>
#include <string>
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

  AST ast(m_ModuleID);
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

  if (TokenType::LET == m_CurrToken.m_Type)
  {
    auto result = ParseStatementLet();
    if (result.is_err())
    {
      return Result<std::shared_ptr<Statement>, Diagnostic>(result.unwrap_err());
    }
    return Result<std::shared_ptr<Statement>, Diagnostic>(result.unwrap());
  }

  if (TokenType::IMPORT == m_CurrToken.m_Type)
  {
    auto result = ParseStatementImport();
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

  if (TokenType::SEMICOLON == m_CurrToken.m_Type)
  {
    Next().unwrap();
  }
  else
  {
    if (TokenType::RBRACE != m_CurrToken.m_Type)
    {
      return Result<std::shared_ptr<Statement>, Diagnostic>(Diagnostic(Errno::SYNTAX_ERROR, expression.get()->m_Position, m_ModuleID, DiagnosticSeverity::ERROR, "implicity return expression must be the last in a block, insert ';' at end"));
    }
    return Result<std::shared_ptr<Statement>, Diagnostic>(std::make_shared<StatementReturn>(StatementReturn(expression->m_Position, expression, StatementReturnType::IMPLICITY)));
  }

  return Result<std::shared_ptr<Statement>, Diagnostic>(expression);
}

Result<FunctionParams, Diagnostic> Parser::ParseFunctionParams()
{
  auto expectRes = Expect(TokenType::LPAREN);
  assert(expectRes.is_ok());
  Position position = expectRes.unwrap();

  std::vector<FunctionParam> params;

  while (!IsEof() && TokenType::RPAREN != m_CurrToken.m_Type)
  {
    if (TokenType::IDENTIFIER != m_CurrToken.m_Type)
    {
      return Result<FunctionParams, Diagnostic>(Diagnostic(Errno::SYNTAX_ERROR, m_CurrToken.m_Position, m_ModuleID, DiagnosticSeverity::ERROR, "expect param name or ')'"));
    }
    auto paramIdentifier = std::make_shared<ExpressionIdentifier>(m_CurrToken.m_Position, m_CurrToken.m_Lexeme);
    Position paramPosition = Next().unwrap();
    TypeAnnotationToken typeAnnotation(std::nullopt, std::make_shared<type::Type>(type::Type(type::Base::ANY)));
    if (TokenType::COLON == m_CurrToken.m_Type)
    {
      Expect(TokenType::COLON).unwrap();
      typeAnnotation.m_Type = ParseTypeAnnotation().unwrap();
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
    return Result<std::shared_ptr<StatementFunction>, Diagnostic>(Diagnostic(Errno::SYNTAX_ERROR, m_CurrToken.m_Position, m_ModuleID, DiagnosticSeverity::ERROR, "expect identifier after 'fn'"));
  }
  auto name = std::make_shared<ExpressionIdentifier>(m_CurrToken.m_Position, m_CurrToken.m_Lexeme);
  Next();
  auto paramsRes = ParseFunctionParams();
  TypeAnnotationToken typeAnnotation(std::nullopt, std::make_shared<type::Type>(type::Type(type::Base::ANY)));
  if (TokenType::COLON == m_CurrToken.m_Type)
  {
    Expect(TokenType::COLON).unwrap();
    typeAnnotation.m_Type = ParseTypeAnnotation().unwrap();
    typeAnnotation.m_Position = Next().unwrap();
  }
  if (paramsRes.is_err())
  {
    return Result<std::shared_ptr<StatementFunction>, Diagnostic>(paramsRes.unwrap_err());
  }
  auto bodyRes = ParseStatementBlock();
  if (bodyRes.is_err())
  {
    return Result<std::shared_ptr<StatementFunction>, Diagnostic>(bodyRes.unwrap_err());
  }
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

Result<std::shared_ptr<StatementLet>, Diagnostic> Parser::ParseStatementLet()
{
  Position position = Expect(TokenType::LET).unwrap();
  // var name
  auto identifierRes = ParseExpressionIdentifier();
  if (identifierRes.is_err())
  {
    return Result<std::shared_ptr<StatementLet>, Diagnostic>(identifierRes.unwrap_err());
  }
  auto identifier = identifierRes.unwrap();
  position.m_End = identifier.get()->m_Position.m_End;
  // var type
  TypeAnnotationToken typeAnnotation(std::nullopt, std::make_shared<type::Type>(type::Type(type::Base::ANY)));
  if (TokenType::COLON == m_CurrToken.m_Type)
  {
    Expect(TokenType::COLON).unwrap();
    typeAnnotation.m_Type = ParseTypeAnnotation().unwrap();
    typeAnnotation.m_Position = Next().unwrap();
  }
  // init value
  std::optional<std::shared_ptr<Expression>> initializerOpt;
  if (TokenType::EQUAL == m_CurrToken.m_Type)
  {
    Expect(TokenType::EQUAL).unwrap();
    auto initializerRes = ParseExpression(Precedence::LOWEST);
    if (initializerRes.is_err())
    {
      return Result<std::shared_ptr<StatementLet>, Diagnostic>(initializerRes.unwrap_err());
    }
    initializerOpt = initializerRes.unwrap();
    position.m_End = initializerOpt.value().get()->m_Position.m_End;
  }
  Expect(TokenType::SEMICOLON).unwrap();
  return Result<std::shared_ptr<StatementLet>, Diagnostic>(std::make_shared<StatementLet>(StatementLet(position, identifier, typeAnnotation, initializerOpt)));
}

Result<std::shared_ptr<StatementImport>, Diagnostic> Parser::ParseStatementImport()
{
  Position position = Expect(TokenType::IMPORT).unwrap();
  auto aliasRes = ParseExpressionIdentifier();
  if (aliasRes.is_err())
  {
    return Result<std::shared_ptr<StatementImport>, Diagnostic>(aliasRes.unwrap_err());
  }
  auto alias = aliasRes.unwrap();
  Expect(TokenType::FROM).unwrap();
  if (TokenType::STRING != m_CurrToken.m_Type)
  {
    return Result<std::shared_ptr<StatementImport>, Diagnostic>(Diagnostic(Errno::SYNTAX_ERROR, m_CurrToken.m_Position, m_ModuleID, DiagnosticSeverity::ERROR, "expect module path"));
  }
  std::string path = m_CurrToken.m_Lexeme;
  position.m_End = Next().unwrap().m_End;
  Expect(TokenType::SEMICOLON).unwrap();
  return Result<std::shared_ptr<StatementImport>, Diagnostic>(std::make_shared<StatementImport>(StatementImport(position, alias, path)));
}

Result<std::shared_ptr<StatementReturn>, Diagnostic> Parser::ParseStatementReturn()
{
  Position position = Expect(TokenType::RETURN).unwrap();
  std::optional<std::shared_ptr<Expression>> value = std::nullopt;
  if (TokenType::SEMICOLON != m_CurrToken.m_Type)
  {
    auto valueRes = ParseExpression(Precedence::LOWEST);
    if (valueRes.is_err())
    {
      return Result<std::shared_ptr<StatementReturn>, Diagnostic>(valueRes.unwrap_err());
    }
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
  case TokenType::EQUAL:
    return Precedence::ASSIGN;
  case TokenType::DOT:
    return Precedence::FIELD_ACCESS;
  default:
    return Precedence::LOWEST;
  }
}

Result<std::shared_ptr<Expression>, Diagnostic> Parser::ParseExpression(Precedence prec)
{
  auto lhsRes = ParseExpressionPrimary();
  if (lhsRes.is_err())
  {
    return Result<std::shared_ptr<Expression>, Diagnostic>(lhsRes.unwrap_err());
  }
  auto nextRes = Next();
  if (nextRes.is_err())
  {
    return Result<std::shared_ptr<Expression>, Diagnostic>(nextRes.unwrap_err());
  }
  while (!IsEof() && prec < token2precedence(m_CurrToken.m_Type))
  {
    switch (m_CurrToken.m_Type)
    {
    case TokenType::LPAREN:
    {
      auto callRes = ParseExpressionCall(lhsRes.unwrap());
      if (callRes.is_err())
      {
        return Result<std::shared_ptr<Expression>, Diagnostic>(lhsRes.unwrap_err());
      }
      lhsRes.set_val(callRes.unwrap());
    }
    break;
    case TokenType::EQUAL:
    {
      auto assignRes = ParseExpressionAssign(lhsRes.unwrap());
      if (assignRes.is_err())
      {
        return Result<std::shared_ptr<Expression>, Diagnostic>(lhsRes.unwrap_err());
      }
      lhsRes.set_val(assignRes.unwrap());
    }
    break;
    case TokenType::DOT:
    {
      auto assignRes = ParseExpressionFieldAccess(lhsRes.unwrap());
      if (assignRes.is_err())
      {
        return Result<std::shared_ptr<Expression>, Diagnostic>(lhsRes.unwrap_err());
      }
      lhsRes.set_val(assignRes.unwrap());
    }
    break;
    default:
      goto defer;
    }
  }
defer:
  return Result<std::shared_ptr<Expression>, Diagnostic>(lhsRes.unwrap());
}

Result<std::shared_ptr<Expression>, Diagnostic> Parser::ParseExpressionPrimary()
{
  switch (m_CurrToken.m_Type)
  {
  case TokenType::STRING:
    return Result<std::shared_ptr<Expression>, Diagnostic>(std::make_shared<ExpressionString>(ExpressionString(m_CurrToken.m_Position, m_CurrToken.m_Lexeme)));
  case TokenType::IDENTIFIER:
    return Result<std::shared_ptr<Expression>, Diagnostic>(std::make_shared<ExpressionIdentifier>(ExpressionIdentifier(m_CurrToken.m_Position, m_CurrToken.m_Lexeme)));
  default:
    // TODO: display expression
    return Result<std::shared_ptr<Expression>, Diagnostic>(Diagnostic(Errno::SYNTAX_ERROR, m_CurrToken.m_Position, m_ModuleID, DiagnosticSeverity::ERROR, "invalid left side expression"));
  }
}

Result<std::shared_ptr<ExpressionCall>, Diagnostic> Parser::ParseExpressionCall(std::shared_ptr<Expression> callee)
{
  auto expectRes = Expect(TokenType::LPAREN);
  if (expectRes.is_err())
  {
    return Result<std::shared_ptr<ExpressionCall>, Diagnostic>(expectRes.unwrap_err());
  }
  Position position = callee.get()->m_Position;
  Position argsPosition = expectRes.unwrap();
  std::vector<std::shared_ptr<Expression>> args;
  while (!IsEof() && TokenType::RPAREN != m_CurrToken.m_Type)
  {
    auto exprRes = ParseExpression(Precedence::LOWEST);
    if (exprRes.is_err())
    {
      return Result<std::shared_ptr<ExpressionCall>, Diagnostic>(exprRes.unwrap_err());
    }
    args.push_back(exprRes.unwrap());
    if (TokenType::RPAREN != m_CurrToken.m_Type)
    {
      expectRes = Expect(TokenType::COMMA);
      if (expectRes.is_err())
      {
        return Result<std::shared_ptr<ExpressionCall>, Diagnostic>(expectRes.unwrap_err());
      }
    }
  }
  expectRes = Expect(TokenType::RPAREN);
  if (expectRes.is_err())
  {
    return Result<std::shared_ptr<ExpressionCall>, Diagnostic>(expectRes.unwrap_err());
  }
  position.m_End = expectRes.unwrap().m_End;
  argsPosition.m_End = expectRes.unwrap().m_End;
  return Result<std::shared_ptr<ExpressionCall>, Diagnostic>(std::make_shared<ExpressionCall>(position, callee, args, argsPosition));
}

Result<std::shared_ptr<ExpressionAssign>, Diagnostic> Parser::ParseExpressionAssign(std::shared_ptr<Expression> assignee)
{
  Position position = assignee.get()->m_Position;
  position.m_End = Expect(TokenType::EQUAL).unwrap().m_End;
  if (ExpressionType::IDENTIFIER != assignee.get()->m_Type)
  {
    return Result<std::shared_ptr<ExpressionAssign>, Diagnostic>(Diagnostic(Errno::SYNTAX_ERROR, position, m_ModuleID, DiagnosticSeverity::ERROR, "can only assign to identifier"));
  }
  auto valueRes = ParseExpression(Precedence::LOWEST);
  if (valueRes.is_err())
  {
    return Result<std::shared_ptr<ExpressionAssign>, Diagnostic>(valueRes.unwrap_err());
  }
  position.m_End = valueRes.unwrap().get()->m_Position.m_End;
  return Result<std::shared_ptr<ExpressionAssign>, Diagnostic>(std::make_shared<ExpressionAssign>(ExpressionAssign(position, std::static_pointer_cast<ExpressionIdentifier>(assignee), valueRes.unwrap())));
}

Result<std::shared_ptr<ExpressionFieldAccess>, Diagnostic> Parser::ParseExpressionFieldAccess(std::shared_ptr<Expression> value)
{
  Position position = value.get()->m_Position;
  Expect(TokenType::DOT).unwrap();
  auto fieldNameRes = ParseExpressionIdentifier();
  if (fieldNameRes.is_err())
  {
    return Result<std::shared_ptr<ExpressionFieldAccess>, Diagnostic>(fieldNameRes.unwrap_err());
  }
  position.m_End = fieldNameRes.unwrap().get()->m_Position.m_End;
  return Result<std::shared_ptr<ExpressionFieldAccess>, Diagnostic>(std::make_shared<ExpressionFieldAccess>(ExpressionFieldAccess(position, value, fieldNameRes.unwrap())));
}

Result<std::shared_ptr<ExpressionIdentifier>, Diagnostic> Parser::ParseExpressionIdentifier()
{
  if (TokenType::IDENTIFIER != m_CurrToken.m_Type)
  {
    return Result<std::shared_ptr<ExpressionIdentifier>, Diagnostic>(Diagnostic(Errno::SYNTAX_ERROR, m_CurrToken.m_Position, m_ModuleID, DiagnosticSeverity::ERROR, "expect an idetifier"));
  }
  auto identifierExpression = std::make_shared<ExpressionIdentifier>(ExpressionIdentifier(m_CurrToken.m_Position, m_CurrToken.m_Lexeme));
  Next().unwrap();
  return Result<std::shared_ptr<ExpressionIdentifier>, Diagnostic>(identifierExpression);
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
    return Result<std::shared_ptr<type::Type>, Diagnostic>(Diagnostic(Errno::SYNTAX_ERROR, m_CurrToken.m_Position, m_ModuleID, DiagnosticSeverity::ERROR, "expect type annotation, try 'i32', 'string', ..."));
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
    return Result<Position, Diagnostic>(Diagnostic(Errno::SYNTAX_ERROR, m_CurrToken.m_Position, m_ModuleID, DiagnosticSeverity::ERROR, "syntax error: expect {} but got {}."));
  }
  return Next();
}

bool Parser::IsEof()
{
  return TokenType::EOf == m_CurrToken.m_Type;
}
