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
    auto stmtRes = ParseStmt();
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

Result<std::shared_ptr<Stmt>, Diagnostic> Parser::ParseStmt()
{
  auto diagnostic = ParsePubAccMod();
  if (diagnostic.has_value())
  {
    return diagnostic.value();
  }
  switch (m_CurrToken.m_Type)
  {
  case TokenType::IMPORT:
    return ParseStmtImport();
    break;
  case TokenType::FUN:
    return ParseStmtFunction();
    break;
  case TokenType::LET:
    return ParseStmtLet();
    break;
  case TokenType::RETURN:
    return ParseStmtReturn();
    break;
  default:
    return ParseStmtExpr();
  }
}

Result<std::shared_ptr<Stmt>, Diagnostic> Parser::ParseStmtImport()
{
  Token importToken = m_CurrToken;
  Next().unwrap();
  auto aliasRes = ParseExprIdent();
  if (aliasRes.is_err())
  {
    return aliasRes.unwrap_err();
  }
  assert(TokenType::FROM == m_CurrToken.m_Type);
  Token fromToken = m_CurrToken;
  Next().unwrap();
  std::optional<Token> atToken;
  if (TokenType::AT == m_CurrToken.m_Type)
  {
    atToken = m_CurrToken;
    Next().unwrap();
  }
  std::vector<std::shared_ptr<IdentExpr>> path;
  do
  {
    auto identRes = ParseExprIdent();
    if (identRes.is_err())
    {
      return identRes.unwrap_err();
    }
    path.push_back(identRes.unwrap());
    if (TokenType::SEMICOLON != m_CurrToken.m_Type)
    {
      assert(TokenType::ASSOC == m_CurrToken.m_Type);
      Next().unwrap();
      assert(TokenType::IDENT == m_CurrToken.m_Type);
    }
  } while (!IsEof() && TokenType::SEMICOLON != m_CurrToken.m_Type);
  Expect(TokenType::SEMICOLON).unwrap();
  return Result<std::shared_ptr<Stmt>, Diagnostic>(std::make_shared<ImportStmt>(ImportStmt(importToken, aliasRes.unwrap(), fromToken, atToken, std::move(path))));
}

Result<std::shared_ptr<Stmt>, Diagnostic> Parser::ParseStmtExpr()
{
  auto expressionRes = ParseExpr(Prec::Low);
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
    return Result<std::shared_ptr<Stmt>, Diagnostic>(std::make_shared<RetStmt>(RetStmt(expression)));
  }
  return Result<std::shared_ptr<Stmt>, Diagnostic>(expression);
}

Result<FunParams, Diagnostic> Parser::ParseFunParams()
{
  assert(TokenType::LPAREN == m_CurrToken.m_Type);
  Position position = Next().unwrap();
  std::optional<Ellipsis> varArgsNotation;
  std::vector<FunParam> params;
  while (!IsEof() && TokenType::RPAREN != m_CurrToken.m_Type)
  {
    if (TokenType::ELLIPSIS == m_CurrToken.m_Type)
    {
      varArgsNotation = Ellipsis(m_CurrToken);
      Next().unwrap();
      assert(TokenType::RPAREN == m_CurrToken.m_Type && "var args must be the last");
      break;
    }
    assert(TokenType::IDENT == m_CurrToken.m_Type);
    auto paramIdentifier = std::make_shared<IdentExpr>(m_CurrToken);
    Next().unwrap();
    std::optional<AstType> paramType;
    if (TokenType::COLON == m_CurrToken.m_Type)
    {
      Expect(TokenType::COLON).unwrap();
      paramType = ParseTypeAnn().unwrap();
    }
    params.push_back(FunParam(paramIdentifier, paramType));
    if (TokenType::RPAREN != m_CurrToken.m_Type)
    {
      assert(TokenType::COMMA == m_CurrToken.m_Type);
      Next().unwrap();
    }
  }
  assert(TokenType::RPAREN == m_CurrToken.m_Type);
  position.m_End = Next().unwrap().m_End;
  return FunParams(position, std::move(params), varArgsNotation);
}

Result<std::shared_ptr<Stmt>, Diagnostic> Parser::ParseStmtFunction()
{
  std::optional<PubAccMod> pubModifier;
  if (m_PubAccModToken.has_value())
  {
    pubModifier = PubAccMod(GetAndErasePubAccMod().value());
  }
  assert(TokenType::FUN == m_CurrToken.m_Type);
  FunDecl funDeclarator(m_CurrToken);
  Next().unwrap();
  assert(TokenType::IDENT == m_CurrToken.m_Type);
  auto identifier = std::make_shared<IdentExpr>(IdentExpr(m_CurrToken));
  Next().unwrap();
  auto paramsRes = ParseFunParams();
  std::optional<AstType> returnType;
  if (TokenType::COLON == m_CurrToken.m_Type)
  {
    Next().unwrap();
    returnType = ParseTypeAnn().unwrap();
  }
  auto params = paramsRes.unwrap();
  auto signature = std::make_shared<FunSignStmt>(FunSignStmt(pubModifier, funDeclarator, identifier, params, returnType));
  if (TokenType::SEMICOLON == m_CurrToken.m_Type)
  {
    Next().unwrap();
    return Result<std::shared_ptr<Stmt>, Diagnostic>(signature);
  }
  auto bodyRes = ParseStmtBlock();
  if (bodyRes.is_err())
  {
    return Result<std::shared_ptr<Stmt>, Diagnostic>(bodyRes.unwrap_err());
  }
  return Result<std::shared_ptr<Stmt>, Diagnostic>(std::make_shared<FunStmt>(FunStmt(signature, std::static_pointer_cast<BlockStmt>(bodyRes.unwrap()))));
}

Result<std::shared_ptr<Stmt>, Diagnostic> Parser::ParseStmtBlock()
{
  Position position = Expect(TokenType::LBRACE).unwrap();
  std::vector<std::shared_ptr<Stmt>> statements = {};
  while (!IsEof() && TokenType::RBRACE != m_CurrToken.m_Type)
  {
    auto statementRes = ParseStmt();
    if (statementRes.is_err())
    {
      return Result<std::shared_ptr<Stmt>, Diagnostic>(statementRes.unwrap_err());
    }
    statements.push_back(statementRes.unwrap());
  }
  position.m_End = Expect(TokenType::RBRACE).unwrap().m_End;
  return Result<std::shared_ptr<Stmt>, Diagnostic>(std::make_shared<BlockStmt>(position, std::move(statements)));
}

Result<std::shared_ptr<Stmt>, Diagnostic> Parser::ParseStmtLet()
{
  std::optional<PubAccMod> pubAccessModifier;
  if (m_PubAccModToken.has_value())
  {
    pubAccessModifier = PubAccMod(GetAndErasePubAccMod().value());
  }
  assert(TokenType::LET == m_CurrToken.m_Type);
  LetDecl letDeclarator(m_CurrToken);
  Next().unwrap();
  // var name
  auto identifierRes = ParseExprIdent();
  if (identifierRes.is_err())
  {
    return Result<std::shared_ptr<Stmt>, Diagnostic>(identifierRes.unwrap_err());
  }
  // var type
  std::optional<AstType> varType;
  if (TokenType::COLON == m_CurrToken.m_Type)
  {
    Next().unwrap();
    varType = ParseTypeAnn().unwrap();
  }
  // init value
  std::optional<std::shared_ptr<Expr>> initializerOpt;
  if (TokenType::EQUAL == m_CurrToken.m_Type)
  {
    Next().unwrap();
    auto initializerRes = ParseExpr(Prec::Low);
    if (initializerRes.is_err())
    {
      return Result<std::shared_ptr<Stmt>, Diagnostic>(initializerRes.unwrap_err());
    }
    initializerOpt = initializerRes.unwrap();
  }
  assert(TokenType::SEMICOLON == m_CurrToken.m_Type);
  Next().unwrap();
  return Result<std::shared_ptr<Stmt>, Diagnostic>(std::make_shared<LetStmt>(LetStmt(pubAccessModifier, letDeclarator, identifierRes.unwrap(), varType, initializerOpt)));
}

Result<std::shared_ptr<Stmt>, Diagnostic> Parser::ParseStmtReturn()
{
  assert(TokenType::RETURN == m_CurrToken.m_Type);
  Token returnToken = m_CurrToken;
  Next().unwrap();
  std::optional<std::shared_ptr<Expr>> value(std::nullopt);
  if (TokenType::SEMICOLON != m_CurrToken.m_Type)
  {
    auto valueRes = ParseExpr(Prec::Low);
    if (valueRes.is_err())
    {
      return Result<std::shared_ptr<Stmt>, Diagnostic>(valueRes.unwrap_err());
    }
    value = valueRes.unwrap();
  }
  Expect(TokenType::SEMICOLON).unwrap();
  return Result<std::shared_ptr<Stmt>, Diagnostic>(std::make_shared<RetStmt>(RetStmt(returnToken, value)));
}

Prec token2precedence(TokenType tokenType)
{
  switch (tokenType)
  {
  case TokenType::LPAREN:
    return Prec::Call;
  case TokenType::EQUAL:
    return Prec::Assign;
  case TokenType::DOT:
    return Prec::FieldAcc;
  default:
    return Prec::Low;
  }
}

Result<std::shared_ptr<Expr>, Diagnostic> Parser::ParseExpr(Prec prec)
{
  auto lhsRes = ParseExprPrim();
  if (lhsRes.is_err())
  {
    return Result<std::shared_ptr<Expr>, Diagnostic>(lhsRes.unwrap_err());
  }
  auto nextRes = Next();
  if (nextRes.is_err())
  {
    return Result<std::shared_ptr<Expr>, Diagnostic>(nextRes.unwrap_err());
  }
  while (!IsEof() && prec < token2precedence(m_CurrToken.m_Type))
  {
    switch (m_CurrToken.m_Type)
    {
    case TokenType::LPAREN:
    {
      auto callRes = ParseExprCall(lhsRes.unwrap());
      if (callRes.is_err())
      {
        return Result<std::shared_ptr<Expr>, Diagnostic>(lhsRes.unwrap_err());
      }
      lhsRes.set_val(callRes.unwrap());
    }
    break;
    case TokenType::EQUAL:
    {
      auto assignRes = ParseExprAssign(lhsRes.unwrap());
      if (assignRes.is_err())
      {
        return Result<std::shared_ptr<Expr>, Diagnostic>(lhsRes.unwrap_err());
      }
      lhsRes.set_val(assignRes.unwrap());
    }
    break;
    case TokenType::DOT:
    {
      auto assignRes = ParseExprFieldAcc(lhsRes.unwrap());
      if (assignRes.is_err())
      {
        return Result<std::shared_ptr<Expr>, Diagnostic>(lhsRes.unwrap_err());
      }
      lhsRes.set_val(assignRes.unwrap());
    }
    break;
    default:
      goto defer;
    }
  }
defer:
  return Result<std::shared_ptr<Expr>, Diagnostic>(lhsRes.unwrap());
}

Result<std::shared_ptr<Expr>, Diagnostic> Parser::ParseExprPrim()
{
  switch (m_CurrToken.m_Type)
  {
  case TokenType::STRING:
    return Result<std::shared_ptr<Expr>, Diagnostic>(std::make_shared<StringExpr>(StringExpr(m_CurrToken)));
  case TokenType::IDENT:
    return Result<std::shared_ptr<Expr>, Diagnostic>(std::make_shared<IdentExpr>(IdentExpr(m_CurrToken)));
  default:
    // TODO: display expression
    return Result<std::shared_ptr<Expr>, Diagnostic>(Diagnostic(Errno::SYNTAX_ERROR, m_CurrToken.m_Position, m_ModuleID, DiagnosticSeverity::ERROR, "invalid left side expression"));
  }
}

Result<std::shared_ptr<CallExpr>, Diagnostic> Parser::ParseExprCall(std::shared_ptr<Expr> callee)
{
  assert(TokenType::LPAREN == m_CurrToken.m_Type);
  Position argsPosition = Next().unwrap();
  std::vector<std::shared_ptr<Expr>> args;
  while (!IsEof() && TokenType::RPAREN != m_CurrToken.m_Type)
  {
    auto exprRes = ParseExpr(Prec::Low);
    if (exprRes.is_err())
    {
      return Result<std::shared_ptr<CallExpr>, Diagnostic>(exprRes.unwrap_err());
    }
    args.push_back(exprRes.unwrap());
    if (TokenType::RPAREN != m_CurrToken.m_Type)
    {
      auto expectRes = Expect(TokenType::COMMA);
      if (expectRes.is_err())
      {
        return Result<std::shared_ptr<CallExpr>, Diagnostic>(expectRes.unwrap_err());
      }
    }
  }
  assert(TokenType::RPAREN == m_CurrToken.m_Type);
  argsPosition.m_End = Next().unwrap().m_End;
  return Result<std::shared_ptr<CallExpr>, Diagnostic>(std::make_shared<CallExpr>(callee, CallExprArgs(argsPosition, std::move(args))));
}

Result<std::shared_ptr<AssignExpr>, Diagnostic> Parser::ParseExprAssign(std::shared_ptr<Expr> assignee)
{
  assert(TokenType::EQUAL == m_CurrToken.m_Type);
  Next().unwrap();
  assert(ExprT::Ident == assignee.get()->GetType());
  auto valueRes = ParseExpr(Prec::Low);
  if (valueRes.is_err())
  {
    return Result<std::shared_ptr<AssignExpr>, Diagnostic>(valueRes.unwrap_err());
  }
  return Result<std::shared_ptr<AssignExpr>, Diagnostic>(std::make_shared<AssignExpr>(AssignExpr(std::static_pointer_cast<IdentExpr>(assignee), valueRes.unwrap())));
}

Result<std::shared_ptr<FieldAccExpr>, Diagnostic> Parser::ParseExprFieldAcc(std::shared_ptr<Expr> value)
{
  Expect(TokenType::DOT).unwrap();
  auto fieldNameRes = ParseExprIdent();
  if (fieldNameRes.is_err())
  {
    return Result<std::shared_ptr<FieldAccExpr>, Diagnostic>(fieldNameRes.unwrap_err());
  }
  return Result<std::shared_ptr<FieldAccExpr>, Diagnostic>(std::make_shared<FieldAccExpr>(FieldAccExpr(value, fieldNameRes.unwrap())));
}

Result<std::shared_ptr<IdentExpr>, Diagnostic> Parser::ParseExprIdent()
{
  if (TokenType::IDENT != m_CurrToken.m_Type)
  {
    return Result<std::shared_ptr<IdentExpr>, Diagnostic>(Diagnostic(Errno::SYNTAX_ERROR, m_CurrToken.m_Position, m_ModuleID, DiagnosticSeverity::ERROR, "expect an idetifier"));
  }
  auto identifierExpression = std::make_shared<IdentExpr>(IdentExpr(m_CurrToken));
  Next().unwrap();
  return Result<std::shared_ptr<IdentExpr>, Diagnostic>(identifierExpression);
}

Result<AstType, Diagnostic> Parser::ParseTypeAnn()
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
    return ParseFunTypeAnn();
  default:
    return Result<AstType, Diagnostic>(Diagnostic(Errno::SYNTAX_ERROR, m_CurrToken.m_Position, m_ModuleID, DiagnosticSeverity::ERROR, "expect type annotation, try 'i32', 'string', ..."));
  }
  return Result<AstType, Diagnostic>(AstType(Next().unwrap(), type));
}

Result<AstType, Diagnostic> Parser::ParseFunTypeAnn()
{
  Position position = Expect(TokenType::FUN).unwrap();
  Expect(TokenType::LPAREN).unwrap();
  std::vector<std::shared_ptr<type::Type>> argsTypes;
  while (!IsEof() && TokenType::RPAREN != m_CurrToken.m_Type)
  {
    auto typeRes = ParseTypeAnn();
    if (typeRes.is_err())
    {
      return Result<AstType, Diagnostic>(typeRes.unwrap_err());
    }
    argsTypes.push_back(typeRes.unwrap().GetType());
  }
  Expect(TokenType::RPAREN).unwrap();
  Expect(TokenType::ARROW).unwrap();
  auto returnType = ParseTypeAnn().unwrap();
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
