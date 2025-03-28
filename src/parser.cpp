#include <cassert>
#include <cstddef>
#include <optional>
#include <string>
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
  auto ast = new AST();
  while (!IsEof())
  {
    auto stmtRes = ParseStmt();
    if (stmtRes.is_ok())
    {
      ast->m_Program.push_back(stmtRes.unwrap());
    }
    else
    {
      return stmtRes.unwrap_err();
    }
  }
  m_Module->m_AST = ast;
  return std::nullopt;
}

Result<bool, Diagnostic> Parser::ParsePubAccMod()
{
  if (TokenType::PUB == m_CurrToken.m_Type)
  {
    m_HasPubModifier = true;
    if (!AcceptsPubModifier(m_NextToken.m_Type))
    {
      return Diagnostic(Errno::SYNTAX_ERROR, m_CurrToken.m_Position, m_ModuleID, DiagnosticSeverity::ERROR, "unexpected 'pub' modifier");
    }
    Next().unwrap();
  }
  return false;
}

bool Parser::EraseIfPubModifier()
{
  if (m_HasPubModifier)
  {
    m_HasPubModifier = false;
    return true;
  }
  return false;
}

Result<Stmt *, Diagnostic> Parser::ParseStmt()
{
  auto result = ParsePubAccMod();
  if (result.is_err())
  {
    return result.unwrap_err();
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

Result<Stmt *, Diagnostic> Parser::ParseStmtImport()
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
  std::vector<IdentExpr *> path;
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
  return new ImportStmt(importToken, aliasRes.unwrap(), fromToken, atToken, std::move(path));
}

Result<Stmt *, Diagnostic> Parser::ParseStmtExpr()
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
      return Diagnostic(Errno::SYNTAX_ERROR, expression->GetPosition(), m_ModuleID, DiagnosticSeverity::ERROR, "implicity return expression must be the last in a block, insert ';' at end");
    }
    return new RetStmt(expression);
  }
  return expression;
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
    auto paramIdentifier = ParseExprIdent().unwrap();
    Expect(TokenType::COLON).unwrap();
    AstType *paramType = ParseTypeAnn().unwrap();
    params.push_back(FunParam(paramIdentifier, paramType));
    if (TokenType::RPAREN != m_CurrToken.m_Type)
    {
      assert(TokenType::COMMA == m_CurrToken.m_Type);
      Next().unwrap();
      assert(m_CurrToken.m_Type == TokenType::IDENT);
    }
  }
  assert(TokenType::RPAREN == m_CurrToken.m_Type);
  position.m_End = Next().unwrap().m_End;
  return FunParams(position, std::move(params), varArgsNotation);
}

Result<Stmt *, Diagnostic> Parser::ParseStmtFunction()
{
  bool isPub = EraseIfPubModifier();
  auto pos = Expect(TokenType::FUN).unwrap();
  auto ident = ParseExprIdent().unwrap();
  auto paramsRes = ParseFunParams();
  AstType *returnType = nullptr;
  if (TokenType::COLON == m_CurrToken.m_Type)
  {
    Next().unwrap();
    returnType = ParseTypeAnn().unwrap();
  }
  auto params = paramsRes.unwrap();
  auto signature = FunSign(isPub, pos, ident, params, returnType);
  if (TokenType::SEMICOLON == m_CurrToken.m_Type)
  {
    Next().unwrap();
    return new FunStmt(signature, nullptr);
  }
  auto bodyRes = ParseStmtBlock();
  if (bodyRes.is_err())
  {
    return bodyRes.unwrap_err();
  }
  return new FunStmt(signature, (BlockStmt *)(bodyRes.unwrap()));
}

Result<Stmt *, Diagnostic> Parser::ParseStmtBlock()
{
  Position position = Expect(TokenType::LBRACE).unwrap();
  std::vector<Stmt *> statements = {};
  while (!IsEof() && TokenType::RBRACE != m_CurrToken.m_Type)
  {
    auto statementRes = ParseStmt();
    if (statementRes.is_err())
    {
      return statementRes.unwrap_err();
    }
    statements.push_back(statementRes.unwrap());
  }
  position.m_End = Expect(TokenType::RBRACE).unwrap().m_End;
  return new BlockStmt(position, std::move(statements));
}

Result<Stmt *, Diagnostic> Parser::ParseStmtLet()
{
  bool isPub = EraseIfPubModifier();
  Position pos = Expect(TokenType::LET).unwrap();
  // var name
  auto identRes = ParseExprIdent();
  if (identRes.is_err())
  {
    identRes.unwrap_err();
  }
  // var type
  AstType *varType = nullptr;
  if (TokenType::COLON == m_CurrToken.m_Type)
  {
    Next().unwrap();
    varType = ParseTypeAnn().unwrap();
  }
  // init value
  Expr *init = nullptr;
  if (TokenType::EQUAL == m_CurrToken.m_Type)
  {
    Next().unwrap();
    auto initializerRes = ParseExpr(Prec::Low);
    if (initializerRes.is_err())
    {
      return Result<Stmt *, Diagnostic>(initializerRes.unwrap_err());
    }
    init = initializerRes.unwrap();
  }
  Expect(TokenType::SEMICOLON).unwrap();
  return new LetStmt(isPub, pos, identRes.unwrap(), varType, init);
}

Result<Stmt *, Diagnostic> Parser::ParseStmtReturn()
{
  assert(TokenType::RETURN == m_CurrToken.m_Type);
  Position pos = Next().unwrap();
  Expr *value = nullptr;
  if (TokenType::SEMICOLON != m_CurrToken.m_Type)
  {
    auto valueRes = ParseExpr(Prec::Low);
    if (valueRes.is_err())
    {
      return Result<Stmt *, Diagnostic>(valueRes.unwrap_err());
    }
    value = valueRes.unwrap();
  }
  Expect(TokenType::SEMICOLON).unwrap();
  return new RetStmt(pos, value);
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

Result<Expr *, Diagnostic> Parser::ParseExpr(Prec prec)
{
  auto lhsRes = ParseExprPrim();
  if (lhsRes.is_err())
  {
    return lhsRes.unwrap_err();
  }
  auto nextRes = Next();
  if (nextRes.is_err())
  {
    return nextRes.unwrap_err();
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
        return lhsRes.unwrap_err();
      }
      lhsRes.set_val(callRes.unwrap());
    }
    break;
    case TokenType::EQUAL:
    {
      auto assignRes = ParseExprAssign(lhsRes.unwrap());
      if (assignRes.is_err())
      {
        lhsRes.unwrap_err();
      }
      lhsRes.set_val(assignRes.unwrap());
    }
    break;
    case TokenType::DOT:
    {
      auto assignRes = ParseExprFieldAcc(lhsRes.unwrap());
      if (assignRes.is_err())
      {
        return lhsRes.unwrap_err();
      }
      lhsRes.set_val(assignRes.unwrap());
    }
    break;
    default:
      goto defer;
    }
  }
defer:
  return lhsRes.unwrap();
}

Result<Expr *, Diagnostic> Parser::ParseExprPrim()
{
  switch (m_CurrToken.m_Type)
  {
  case TokenType::STRING:
    return new StringExpr(m_CurrToken);
  case TokenType::IDENT:
    return new IdentExpr(m_CurrToken);
  default:
    // TODO: display expression
    return Diagnostic(Errno::SYNTAX_ERROR, m_CurrToken.m_Position, m_ModuleID, DiagnosticSeverity::ERROR, "invalid left side expression");
  }
}

Result<CallExpr *, Diagnostic> Parser::ParseExprCall(Expr *callee)
{
  assert(TokenType::LPAREN == m_CurrToken.m_Type);
  Position argsPosition = Next().unwrap();
  std::vector<Expr *> args;
  while (!IsEof() && TokenType::RPAREN != m_CurrToken.m_Type)
  {
    auto exprRes = ParseExpr(Prec::Low);
    if (exprRes.is_err())
    {
      return exprRes.unwrap_err();
    }
    args.push_back(exprRes.unwrap());
    if (TokenType::RPAREN != m_CurrToken.m_Type)
    {
      auto expectRes = Expect(TokenType::COMMA);
      if (expectRes.is_err())
      {
        return expectRes.unwrap_err();
      }
    }
  }
  assert(TokenType::RPAREN == m_CurrToken.m_Type);
  argsPosition.m_End = Next().unwrap().m_End;
  return new CallExpr(callee, CallExprArgs(argsPosition, std::move(args)));
}

Result<AssignExpr *, Diagnostic> Parser::ParseExprAssign(Expr *dest)
{
  assert(TokenType::EQUAL == m_CurrToken.m_Type);
  Next().unwrap();
  assert(ExprT::Ident == dest->GetType());
  auto valueRes = ParseExpr(Prec::Low);
  if (valueRes.is_err())
  {
    return valueRes.unwrap_err();
  }
  return new AssignExpr((IdentExpr *)dest, valueRes.unwrap());
}

Result<FieldAccExpr *, Diagnostic> Parser::ParseExprFieldAcc(Expr *value)
{
  Expect(TokenType::DOT).unwrap();
  auto fieldNameRes = ParseExprIdent();
  if (fieldNameRes.is_err())
  {
    return fieldNameRes.unwrap_err();
  }
  return new FieldAccExpr(value, fieldNameRes.unwrap());
}

Result<IdentExpr *, Diagnostic> Parser::ParseExprIdent()
{
  if (TokenType::IDENT != m_CurrToken.m_Type)
  {
    return Result<IdentExpr *, Diagnostic>(Diagnostic(Errno::SYNTAX_ERROR, m_CurrToken.m_Position, m_ModuleID, DiagnosticSeverity::ERROR, "expect an idetifier"));
  }
  auto identifierExpression = new IdentExpr(m_CurrToken);
  Next().unwrap();
  return Result<IdentExpr *, Diagnostic>(identifierExpression);
}

Result<AstType *, Diagnostic> Parser::ParseTypeAnn()
{
  type::Type *type;
  switch (m_CurrToken.m_Type)
  {
  case TokenType::I8:
    type = new type::Type(type::Base::I8);
    break;
  case TokenType::I16:
    type = new type::Type(type::Base::I16);
    break;
  case TokenType::I32:
    type = new type::Type(type::Base::I32);
    break;
  case TokenType::I64:
    type = new type::Type(type::Base::I64);
    break;
  case TokenType::U8:
    type = new type::Type(type::Base::U8);
    break;
  case TokenType::U16:
    type = new type::Type(type::Base::U16);
    break;
  case TokenType::U32:
    type = new type::Type(type::Base::U32);
    break;
  case TokenType::U64:
    type = new type::Type(type::Base::U32);
    break;
  case TokenType::F32:
    type = new type::Type(type::Base::F32);
    break;
  case TokenType::F64:
    type = new type::Type(type::Base::F64);
    break;
  case TokenType::VOID:
    type = new type::Type(type::Base::VOID);
    break;
  case TokenType::STR_TYP:
    type = new type::Type(type::Base::STRING);
    break;
  case TokenType::FUN:
    return ParseFunTypeAnn();
  default:
    return Diagnostic(Errno::SYNTAX_ERROR, m_CurrToken.m_Position, m_ModuleID, DiagnosticSeverity::ERROR, "expect type annotation, try 'i32', 'string', ...");
  }
  return new AstType(Next().unwrap(), type);
}

Result<AstType *, Diagnostic> Parser::ParseFunTypeAnn()
{
  Position position = Expect(TokenType::FUN).unwrap();
  Expect(TokenType::LPAREN).unwrap();
  std::vector<type::Type *> argsTypes;
  while (!IsEof() && TokenType::RPAREN != m_CurrToken.m_Type)
  {
    auto typeRes = ParseTypeAnn();
    if (typeRes.is_err())
    {
      return typeRes.unwrap_err();
    }
    argsTypes.push_back(typeRes.unwrap()->GetType());
  }
  Expect(TokenType::RPAREN).unwrap();
  Expect(TokenType::ARROW).unwrap();
  auto returnType = ParseTypeAnn().unwrap();
  position.m_End = returnType->GetPosition().m_End;
  size_t argsCount = argsTypes.size();
  auto functionType = new type::Function(argsCount, std::move(argsTypes), returnType->GetType());
  return new AstType(position, functionType);
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

bool Parser::AcceptsPubModifier(TokenType tokenType)
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
