#include <cassert>
#include <cmath>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "ast.h"
#include "diagnostic.h"
#include "parser.h"
#include "result.h"
#include "token.h"
#include "type.h"

std::optional<Diagnostic> Parser::Parse()
{
  Next().unwrap();
  Next().unwrap();
  auto ast = std::make_shared<AST>(AST());
  while (!IsEof())
  {
    auto stmtRes = ParseStatement();
    if (stmtRes.is_ok())
    {
      ast.get()->m_Program.push_back(stmtRes.unwrap());
    }
    else
    {
      return stmtRes.unwrap_err();
    }
  }
  m_Module.get()->m_AST = ast;
  return std::nullopt;
}

std::optional<Diagnostic> Parser::ParsePubAccMod()
{
  if (TokenType::PUB == m_CurrToken.m_Type)
  {
    m_PubAccModToken = m_CurrToken;
    if (!AcceptsPubAccMod(m_NextToken.m_Type))
    {
      return Diagnostic(Errno::SYNTAX_ERROR, m_CurrToken.m_Position, m_ModuleID, DiagnosticSeverity::ERROR, "unexpected 'pub' modifier");
    }
    Next().unwrap();
  }
  return std::nullopt;
}

Result<std::shared_ptr<Statement>, Diagnostic> Parser::ParseStatement()
{
  auto diagnostic = ParsePubAccMod();
  if (diagnostic.has_value())
  {
    return diagnostic.value();
  }
  Result<std::shared_ptr<Statement>, Diagnostic> result(nullptr);
  switch (m_CurrToken.m_Type)
  {
  case TokenType::FUN:
    result = ParseStatementFunction();
    break;
  case TokenType::LET:
    result = ParseStatementLet();
    break;
  case TokenType::RETURN:
    result = ParseStatementReturn();
    break;
  default:
    result = ParseStatementExpression();
  }
  return result;
}

Result<std::shared_ptr<Statement>, Diagnostic> Parser::ParseStatementExpression()
{
  auto expressionRes = ParseExpression(Precedence::LOWEST);
  if (expressionRes.is_err())
  {
    return expressionRes.unwrap_err();
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
      return Diagnostic(Errno::SYNTAX_ERROR, expression.get()->GetPosition(), m_ModuleID, DiagnosticSeverity::ERROR, "implicity return expression must be the last in a block, insert ';' at end");
    }
    return Result<std::shared_ptr<Statement>, Diagnostic>(std::make_shared<StatementReturn>(StatementReturn(expression)));
  }
  return Result<std::shared_ptr<Statement>, Diagnostic>(expression);
}

Result<FunctionParams, Diagnostic> Parser::ParseFunctionParams()
{
  assert(TokenType::LPAREN == m_CurrToken.m_Type);
  Position position = Next().unwrap();
  std::optional<VarArgsNotation> varArgsNotation;
  std::vector<FunctionParam> params;
  while (!IsEof() && TokenType::RPAREN != m_CurrToken.m_Type)
  {
    if (TokenType::ELLIPSIS == m_CurrToken.m_Type)
    {
      varArgsNotation = VarArgsNotation(m_CurrToken);
      Next().unwrap();
      assert(TokenType::RPAREN == m_CurrToken.m_Type && "var args must be the last");
      break;
    }
    assert(TokenType::IDENT == m_CurrToken.m_Type);
    auto paramIdentifier = std::make_shared<ExpressionIdentifier>(m_CurrToken);
    Next().unwrap();
    std::optional<AstType> paramType;
    if (TokenType::COLON == m_CurrToken.m_Type)
    {
      Expect(TokenType::COLON).unwrap();
      paramType = ParseTypeAnnotation().unwrap();
    }
    params.push_back(FunctionParam(paramIdentifier, paramType));
    if (TokenType::RPAREN != m_CurrToken.m_Type)
    {
      assert(TokenType::COMMA == m_CurrToken.m_Type);
      Next().unwrap();
    }
  }
  assert(TokenType::RPAREN == m_CurrToken.m_Type);
  position.m_End = Next().unwrap().m_End;
  return FunctionParams(position, std::move(params), varArgsNotation);
}

Result<std::shared_ptr<Statement>, Diagnostic> Parser::ParseStatementFunction()
{
  std::optional<PubAccessModifier> pubModifier;
  if (m_PubAccModToken.has_value())
  {
    pubModifier = PubAccessModifier(GetAndErasePubAccMod().value());
  }
  assert(TokenType::FUN == m_CurrToken.m_Type);
  FunDeclarator funDeclarator(m_CurrToken);
  Next().unwrap();
  assert(TokenType::IDENT == m_CurrToken.m_Type);
  auto identifier = std::make_shared<ExpressionIdentifier>(ExpressionIdentifier(m_CurrToken));
  Next().unwrap();
  auto paramsRes = ParseFunctionParams();
  std::optional<AstType> returnType;
  if (TokenType::COLON == m_CurrToken.m_Type)
  {
    Next().unwrap();
    returnType = ParseTypeAnnotation().unwrap();
  }
  auto params = paramsRes.unwrap();
  auto signature = std::make_shared<StatementFunctionSignature>(StatementFunctionSignature(pubModifier, funDeclarator, identifier, params, returnType));
  if (TokenType::SEMICOLON == m_CurrToken.m_Type)
  {
    Next().unwrap();
    return Result<std::shared_ptr<Statement>, Diagnostic>(signature);
  }
  auto bodyRes = ParseStatementBlock();
  if (bodyRes.is_err())
  {
    return Result<std::shared_ptr<Statement>, Diagnostic>(bodyRes.unwrap_err());
  }
  return Result<std::shared_ptr<Statement>, Diagnostic>(std::make_shared<StatementFunction>(StatementFunction(signature, std::static_pointer_cast<StatementBlock>(bodyRes.unwrap()))));
}

Result<std::shared_ptr<Statement>, Diagnostic> Parser::ParseStatementBlock()
{
  Position position = Expect(TokenType::LBRACE).unwrap();
  std::vector<std::shared_ptr<Statement>> statements = {};
  while (!IsEof() && TokenType::RBRACE != m_CurrToken.m_Type)
  {
    auto statementRes = ParseStatement();
    if (statementRes.is_err())
    {
      return Result<std::shared_ptr<Statement>, Diagnostic>(statementRes.unwrap_err());
    }
    statements.push_back(statementRes.unwrap());
  }
  position.m_End = Expect(TokenType::RBRACE).unwrap().m_End;
  return Result<std::shared_ptr<Statement>, Diagnostic>(std::make_shared<StatementBlock>(position, std::move(statements)));
}

Result<std::shared_ptr<Statement>, Diagnostic> Parser::ParseStatementLet()
{
  std::optional<PubAccessModifier> pubAccessModifier;
  if (m_PubAccModToken.has_value())
  {
    pubAccessModifier = PubAccessModifier(GetAndErasePubAccMod().value());
  }
  assert(TokenType::LET == m_CurrToken.m_Type);
  LetDeclarator letDeclarator(m_CurrToken);
  Next().unwrap();
  // var name
  auto identifierRes = ParseExpressionIdentifier();
  if (identifierRes.is_err())
  {
    return Result<std::shared_ptr<Statement>, Diagnostic>(identifierRes.unwrap_err());
  }
  // var type
  std::optional<AstType> varType;
  if (TokenType::COLON == m_CurrToken.m_Type)
  {
    Next().unwrap();
    varType = ParseTypeAnnotation().unwrap();
  }
  // init value
  std::optional<std::shared_ptr<Expression>> initializerOpt;
  if (TokenType::EQUAL == m_CurrToken.m_Type)
  {
    Next().unwrap();
    auto initializerRes = ParseExpression(Precedence::LOWEST);
    if (initializerRes.is_err())
    {
      return Result<std::shared_ptr<Statement>, Diagnostic>(initializerRes.unwrap_err());
    }
    initializerOpt = initializerRes.unwrap();
  }
  assert(TokenType::SEMICOLON == m_CurrToken.m_Type);
  Next().unwrap();
  return Result<std::shared_ptr<Statement>, Diagnostic>(std::make_shared<StatementLet>(StatementLet(pubAccessModifier, letDeclarator, identifierRes.unwrap(), varType, initializerOpt)));
}

Result<std::shared_ptr<Statement>, Diagnostic> Parser::ParseStatementReturn()
{
  assert(TokenType::RETURN == m_CurrToken.m_Type);
  Token returnToken = m_CurrToken;
  Next().unwrap();
  std::optional<std::shared_ptr<Expression>> value(std::nullopt);
  if (TokenType::SEMICOLON != m_CurrToken.m_Type)
  {
    auto valueRes = ParseExpression(Precedence::LOWEST);
    if (valueRes.is_err())
    {
      return Result<std::shared_ptr<Statement>, Diagnostic>(valueRes.unwrap_err());
    }
    value = valueRes.unwrap();
  }
  Expect(TokenType::SEMICOLON).unwrap();
  return Result<std::shared_ptr<Statement>, Diagnostic>(std::make_shared<StatementReturn>(StatementReturn(returnToken, value)));
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
    return Result<std::shared_ptr<Expression>, Diagnostic>(std::make_shared<ExpressionString>(ExpressionString(m_CurrToken)));
  case TokenType::IDENT:
    return Result<std::shared_ptr<Expression>, Diagnostic>(std::make_shared<ExpressionIdentifier>(ExpressionIdentifier(m_CurrToken)));
  default:
    // TODO: display expression
    return Result<std::shared_ptr<Expression>, Diagnostic>(Diagnostic(Errno::SYNTAX_ERROR, m_CurrToken.m_Position, m_ModuleID, DiagnosticSeverity::ERROR, "invalid left side expression"));
  }
}

Result<std::shared_ptr<ExpressionCall>, Diagnostic> Parser::ParseExpressionCall(std::shared_ptr<Expression> callee)
{
  assert(TokenType::LPAREN == m_CurrToken.m_Type);
  Position argsPosition = Next().unwrap();
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
      auto expectRes = Expect(TokenType::COMMA);
      if (expectRes.is_err())
      {
        return Result<std::shared_ptr<ExpressionCall>, Diagnostic>(expectRes.unwrap_err());
      }
    }
  }
  assert(TokenType::RPAREN == m_CurrToken.m_Type);
  argsPosition.m_End = Next().unwrap().m_End;
  return Result<std::shared_ptr<ExpressionCall>, Diagnostic>(std::make_shared<ExpressionCall>(callee, ExpressionCallArguments(argsPosition, std::move(args))));
}

Result<std::shared_ptr<ExpressionAssign>, Diagnostic> Parser::ParseExpressionAssign(std::shared_ptr<Expression> assignee)
{
  assert(TokenType::EQUAL == m_CurrToken.m_Type);
  Next().unwrap();
  assert(ExpressionType::IDENTIFIER == assignee.get()->GetType());
  auto valueRes = ParseExpression(Precedence::LOWEST);
  if (valueRes.is_err())
  {
    return Result<std::shared_ptr<ExpressionAssign>, Diagnostic>(valueRes.unwrap_err());
  }
  return Result<std::shared_ptr<ExpressionAssign>, Diagnostic>(std::make_shared<ExpressionAssign>(ExpressionAssign(std::static_pointer_cast<ExpressionIdentifier>(assignee), valueRes.unwrap())));
}

Result<std::shared_ptr<ExpressionFieldAccess>, Diagnostic> Parser::ParseExpressionFieldAccess(std::shared_ptr<Expression> value)
{
  Expect(TokenType::DOT).unwrap();
  auto fieldNameRes = ParseExpressionIdentifier();
  if (fieldNameRes.is_err())
  {
    return Result<std::shared_ptr<ExpressionFieldAccess>, Diagnostic>(fieldNameRes.unwrap_err());
  }
  return Result<std::shared_ptr<ExpressionFieldAccess>, Diagnostic>(std::make_shared<ExpressionFieldAccess>(ExpressionFieldAccess(value, fieldNameRes.unwrap())));
}

Result<std::shared_ptr<ExpressionIdentifier>, Diagnostic> Parser::ParseExpressionIdentifier()
{
  if (TokenType::IDENT != m_CurrToken.m_Type)
  {
    return Result<std::shared_ptr<ExpressionIdentifier>, Diagnostic>(Diagnostic(Errno::SYNTAX_ERROR, m_CurrToken.m_Position, m_ModuleID, DiagnosticSeverity::ERROR, "expect an idetifier"));
  }
  auto identifierExpression = std::make_shared<ExpressionIdentifier>(ExpressionIdentifier(m_CurrToken));
  Next().unwrap();
  return Result<std::shared_ptr<ExpressionIdentifier>, Diagnostic>(identifierExpression);
}

Result<AstType, Diagnostic> Parser::ParseTypeAnnotation()
{
  std::shared_ptr<type::Type> type;
  switch (m_CurrToken.m_Type)
  {
  case TokenType::I8:
    type = type::Type::make_i8();
    break;
  case TokenType::I16:
    type = type::Type::make_i16();
    break;
  case TokenType::I32:
    type = type::Type::make_i32();
    break;
  case TokenType::I64:
    type = type::Type::make_i64();
    break;
  case TokenType::U8:
    type = type::Type::make_u8();
    break;
  case TokenType::U16:
    type = type::Type::make_u16();
    break;
  case TokenType::U32:
    type = type::Type::make_u32();
    break;
  case TokenType::U64:
    type = type::Type::make_u64();
    break;
  case TokenType::F8:
    type = type::Type::make_f8();
    break;
  case TokenType::F16:
    type = type::Type::make_f16();
    break;
  case TokenType::F32:
    type = type::Type::make_f32();
    break;
  case TokenType::F64:
    type = type::Type::make_f64();
    break;
  case TokenType::VOID:
    type = type::Type::make_void();
    break;
  case TokenType::STR_TYP:
    type = type::Type::make_string();
    break;
  case TokenType::FUN:
    return ParseTypeAnnotationFunction();
  default:
    return Result<AstType, Diagnostic>(Diagnostic(Errno::SYNTAX_ERROR, m_CurrToken.m_Position, m_ModuleID, DiagnosticSeverity::ERROR, "expect type annotation, try 'i32', 'string', ..."));
  }
  return Result<AstType, Diagnostic>(AstType(Next().unwrap(), type));
}

Result<AstType, Diagnostic> Parser::ParseTypeAnnotationFunction()
{
  Position position = Expect(TokenType::FUN).unwrap();
  Expect(TokenType::LPAREN).unwrap();
  std::vector<std::shared_ptr<type::Type>> argsTypes;
  while (!IsEof() && TokenType::RPAREN != m_CurrToken.m_Type)
  {
    auto typeRes = ParseTypeAnnotation();
    if (typeRes.is_err())
    {
      return Result<AstType, Diagnostic>(typeRes.unwrap_err());
    }
    argsTypes.push_back(typeRes.unwrap().GetType());
  }
  Expect(TokenType::RPAREN).unwrap();
  Expect(TokenType::ARROW).unwrap();
  auto returnType = ParseTypeAnnotation().unwrap();
  position.m_End = returnType.GetPosition().m_End;
  size_t argsCount = argsTypes.size();
  auto functionType = std::make_shared<type::Function>(type::Function(argsCount, std::move(argsTypes), returnType.GetType()));
  return Result<AstType, Diagnostic>(AstType(position, functionType));
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

std::optional<Token> Parser::GetAndErasePubAccMod()
{
  if (m_PubAccModToken.has_value())
  {
    auto token = m_PubAccModToken.value();
    m_PubAccModToken.reset();
    return token;
  }
  return std::nullopt;
}

bool Parser::AcceptsPubAccMod(TokenType tokenType)
{
  switch (tokenType)
  {
  case TokenType::FUN:
  case TokenType::CLASS:
  case TokenType::LET:
    return true;
  default:
    return false;
  }
}

bool Parser::IsEof()
{
  return TokenType::END == m_CurrToken.m_Type;
}
