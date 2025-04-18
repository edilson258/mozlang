#include <cassert>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ast.h"
#include "diagnostic.h"
#include "parser.h"
#include "pointer.h"
#include "result.h"
#include "token.h"
#include "type.h"

std::optional<Diagnostic> Parser::Parse()
{
  Next().unwrap();
  Next().unwrap();
  auto ast = MakePtr(Ast());
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
  if (TokenType::Pub == m_CurrToken.m_Type)
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

Result<Ptr<Stmt>, Diagnostic> Parser::ParseStmt()
{
  auto result = ParsePubAccMod();
  if (result.is_err())
  {
    return result.unwrap_err();
  }
  switch (m_CurrToken.m_Type)
  {
  case TokenType::Import:
    return ParseStmtImport();
    break;
  case TokenType::Fun:
    return ParseStmtFunction();
    break;
  case TokenType::Let:
    return ParseStmtLet();
    break;
  case TokenType::Ret:
    return ParseStmtReturn();
    break;
  default:
    return ParseStmtExpr();
  }
}

Result<Ptr<Stmt>, Diagnostic> Parser::ParseStmtImport()
{
  Position pos = Expect(TokenType::Import).unwrap();
  auto aliasRes = ParseExprIdent();
  if (aliasRes.is_err())
  {
    return aliasRes.unwrap_err();
  }
  Expect(TokenType::From).unwrap();
  std::optional<Token> atToken;
  if (TokenType::At == m_CurrToken.m_Type)
  {
    atToken = m_CurrToken;
    Next().unwrap();
  }
  std::vector<Ptr<IdentExpr>> path;
  do
  {
    auto identRes = ParseExprIdent();
    if (identRes.is_err())
    {
      return identRes.unwrap_err();
    }
    path.push_back(identRes.unwrap());
    if (TokenType::Semi != m_CurrToken.m_Type)
    {
      assert(TokenType::Assoc == m_CurrToken.m_Type);
      Next().unwrap();
      assert(TokenType::Ident == m_CurrToken.m_Type);
    }
  } while (!IsEof() && TokenType::Semi != m_CurrToken.m_Type);
  Expect(TokenType::Semi).unwrap();
  return Result<Ptr<Stmt>, Diagnostic>(MakePtr<ImportStmt>(ImportStmt(pos, aliasRes.unwrap(), atToken, std::move(path))));
}

Result<Ptr<Stmt>, Diagnostic> Parser::ParseStmtExpr()
{
  auto expressionRes = ParseExpr(Prec::Low);
  if (expressionRes.is_err())
  {
    return expressionRes.unwrap_err();
  }
  auto expression = expressionRes.unwrap();
  if (TokenType::Semi == m_CurrToken.m_Type)
  {
    Next().unwrap();
  }
  else
  {
    if (TokenType::Rbrace != m_CurrToken.m_Type)
    {
      return Diagnostic(Errno::SYNTAX_ERROR, expression->GetPos(), m_ModuleID, DiagnosticSeverity::ERROR, "implicity return expression must be the last in a block, insert ';' at end");
    }
    return CastPtr<Stmt>(MakePtr<RetStmt>(RetStmt(expression)));
  }
  return CastPtr<Stmt>(expression);
}

Result<FunParams, Diagnostic> Parser::ParseFunParams()
{
  assert(TokenType::Lparen == m_CurrToken.m_Type);
  Position position = Next().unwrap();
  std::optional<Ellipsis> varArgsNotation;
  std::vector<FunParam> params;
  while (!IsEof() && TokenType::Rparen != m_CurrToken.m_Type)
  {
    if (TokenType::Ellipsis == m_CurrToken.m_Type)
    {
      varArgsNotation = Ellipsis(m_CurrToken);
      Next().unwrap();
      assert(TokenType::Rparen == m_CurrToken.m_Type && "var args must be the last");
      break;
    }
    assert(TokenType::Ident == m_CurrToken.m_Type);
    auto paramIdentifier = ParseExprIdent().unwrap();
    Expect(TokenType::Colon).unwrap();
    auto paramType = ParseTypeAnn().unwrap();
    params.push_back(FunParam(paramIdentifier, paramType));
    if (TokenType::Rparen != m_CurrToken.m_Type)
    {
      assert(TokenType::Comma == m_CurrToken.m_Type);
      Next().unwrap();
      assert(m_CurrToken.m_Type == TokenType::Ident);
    }
  }
  assert(TokenType::Rparen == m_CurrToken.m_Type);
  position.m_End = Next().unwrap().m_End;
  return FunParams(position, std::move(params), varArgsNotation);
}

Result<Ptr<Stmt>, Diagnostic> Parser::ParseStmtFunction()
{
  bool isPub = EraseIfPubModifier();
  auto pos = Expect(TokenType::Fun).unwrap();
  auto ident = ParseExprIdent().unwrap();
  auto paramsRes = ParseFunParams();
  Ptr<AstType> returnType = nullptr;
  if (TokenType::Colon == m_CurrToken.m_Type)
  {
    Next().unwrap();
    returnType = ParseTypeAnn().unwrap();
  }
  auto params = paramsRes.unwrap();
  auto signature = FunSign(isPub, pos, ident, params, returnType);
  if (TokenType::Semi == m_CurrToken.m_Type)
  {
    Next().unwrap();
    return CastPtr<Stmt>(MakePtr<FunStmt>(FunStmt(signature, nullptr)));
  }
  auto bodyRes = ParseStmtBlock();
  if (bodyRes.is_err())
  {
    return bodyRes.unwrap_err();
  }
  return CastPtr<Stmt>(MakePtr(FunStmt(signature, CastPtr<BlockStmt>(bodyRes.unwrap()))));
}

Result<Ptr<Stmt>, Diagnostic> Parser::ParseStmtBlock()
{
  Position position = Expect(TokenType::Lbrace).unwrap();
  std::vector<Ptr<Stmt>> statements = {};
  while (!IsEof() && TokenType::Rbrace != m_CurrToken.m_Type)
  {
    auto statementRes = ParseStmt();
    if (statementRes.is_err())
    {
      return statementRes.unwrap_err();
    }
    statements.push_back(statementRes.unwrap());
  }
  position.m_End = Expect(TokenType::Rbrace).unwrap().m_End;
  return CastPtr<Stmt>(MakePtr(BlockStmt(position, std::move(statements))));
}

Result<Ptr<Stmt>, Diagnostic> Parser::ParseStmtLet()
{
  bool isPub = EraseIfPubModifier();
  Position pos = Expect(TokenType::Let).unwrap();
  // var name
  auto identRes = ParseExprIdent();
  if (identRes.is_err())
  {
    identRes.unwrap_err();
  }
  // var type
  Ptr<AstType> varType = nullptr;
  if (TokenType::Colon == m_CurrToken.m_Type)
  {
    Next().unwrap();
    varType = ParseTypeAnn().unwrap();
  }
  // init value
  Ptr<Expr> init = nullptr;
  if (TokenType::Equal == m_CurrToken.m_Type)
  {
    Next().unwrap();
    auto initializerRes = ParseExpr(Prec::Low);
    if (initializerRes.is_err())
    {
      return initializerRes.unwrap_err();
    }
    init = initializerRes.unwrap();
  }
  Expect(TokenType::Semi).unwrap();
  return CastPtr<Stmt>(MakePtr(LetStmt(isPub, pos, identRes.unwrap(), varType, init)));
}

Result<Ptr<Stmt>, Diagnostic> Parser::ParseStmtReturn()
{
  assert(TokenType::Ret == m_CurrToken.m_Type);
  Position pos = Next().unwrap();
  Ptr<Expr> value = nullptr;
  if (TokenType::Semi != m_CurrToken.m_Type)
  {
    auto valueRes = ParseExpr(Prec::Low);
    if (valueRes.is_err())
    {
      return valueRes.unwrap_err();
    }
    value = valueRes.unwrap();
  }
  Expect(TokenType::Semi).unwrap();
  return CastPtr<Stmt>(MakePtr(RetStmt(pos, value)));
}

Prec token2precedence(TokenType tokenType)
{
  switch (tokenType)
  {
  case TokenType::Lparen:
    return Prec::Call;
  case TokenType::Equal:
    return Prec::Assign;
  case TokenType::Dot:
    return Prec::FieldAcc;
  default:
    return Prec::Low;
  }
}

Result<Ptr<Expr>, Diagnostic> Parser::ParseExpr(Prec prec)
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
    case TokenType::Lparen:
    {
      auto callRes = ParseExprCall(lhsRes.unwrap());
      if (callRes.is_err())
      {
        return lhsRes.unwrap_err();
      }
      lhsRes.set_val(callRes.unwrap());
    }
    break;
    case TokenType::Equal:
    {
      auto assignRes = ParseExprAssign(lhsRes.unwrap());
      if (assignRes.is_err())
      {
        lhsRes.unwrap_err();
      }
      lhsRes.set_val(assignRes.unwrap());
    }
    break;
    case TokenType::Dot:
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

Result<Ptr<Expr>, Diagnostic> Parser::ParseExprPrim()
{
  switch (m_CurrToken.m_Type)
  {
  case TokenType::StrLit:
    return CastPtr<Expr>(MakePtr(StringExpr(m_CurrToken)));
  case TokenType::Ident:
    return CastPtr<Expr>(MakePtr(IdentExpr(m_CurrToken)));
  case TokenType::BinLit:
    return CastPtr<Expr>(MakePtr(NumberExpr(m_CurrToken.m_Position, m_CurrToken.m_Lexeme, NumberBase::Bin)));
  case TokenType::DecLit:
    return CastPtr<Expr>(MakePtr(NumberExpr(m_CurrToken.m_Position, m_CurrToken.m_Lexeme, NumberBase::Dec)));
  case TokenType::HexLit:
    return CastPtr<Expr>(MakePtr(NumberExpr(m_CurrToken.m_Position, m_CurrToken.m_Lexeme, NumberBase::Hex)));
  case TokenType::FloatLit:
    return CastPtr<Expr>(MakePtr(NumberExpr(m_CurrToken.m_Position, m_CurrToken.m_Lexeme, NumberBase::Dec, true)));
  default:
    // TODO: display expression
    return Diagnostic(Errno::SYNTAX_ERROR, m_CurrToken.m_Position, m_ModuleID, DiagnosticSeverity::ERROR, "invalid left side expression");
  }
}

Result<Ptr<CallExpr>, Diagnostic> Parser::ParseExprCall(Ptr<Expr> callee)
{
  assert(TokenType::Lparen == m_CurrToken.m_Type);
  Position argsPosition = Next().unwrap();
  std::vector<Ptr<Expr>> args;
  while (!IsEof() && TokenType::Rparen != m_CurrToken.m_Type)
  {
    auto exprRes = ParseExpr(Prec::Low);
    if (exprRes.is_err())
    {
      return exprRes.unwrap_err();
    }
    args.push_back(exprRes.unwrap());
    if (TokenType::Rparen != m_CurrToken.m_Type)
    {
      auto expectRes = Expect(TokenType::Comma);
      if (expectRes.is_err())
      {
        return expectRes.unwrap_err();
      }
    }
  }
  assert(TokenType::Rparen == m_CurrToken.m_Type);
  argsPosition.m_End = Next().unwrap().m_End;
  return MakePtr(CallExpr(callee, CallExprArgs(argsPosition, std::move(args))));
}

Result<Ptr<AssignExpr>, Diagnostic> Parser::ParseExprAssign(Ptr<Expr> dest)
{
  assert(TokenType::Equal == m_CurrToken.m_Type);
  Next().unwrap();
  assert(ExprT::Ident == dest->GetType());
  auto valueRes = ParseExpr(Prec::Low);
  if (valueRes.is_err())
  {
    return valueRes.unwrap_err();
  }
  return MakePtr(AssignExpr(CastPtr<IdentExpr>(dest), valueRes.unwrap()));
}

Result<Ptr<FieldAccExpr>, Diagnostic> Parser::ParseExprFieldAcc(Ptr<Expr> value)
{
  Expect(TokenType::Dot).unwrap();
  auto fieldNameRes = ParseExprIdent();
  if (fieldNameRes.is_err())
  {
    return fieldNameRes.unwrap_err();
  }
  return MakePtr(FieldAccExpr(value, fieldNameRes.unwrap()));
}

Result<Ptr<IdentExpr>, Diagnostic> Parser::ParseExprIdent()
{
  if (TokenType::Ident != m_CurrToken.m_Type)
  {
    return Diagnostic(Errno::SYNTAX_ERROR, m_CurrToken.m_Position, m_ModuleID, DiagnosticSeverity::ERROR, "expect an idetifier");
  }
  auto identifierExpression = MakePtr(IdentExpr(m_CurrToken));
  Next().unwrap();
  return identifierExpression;
}

Result<Ptr<AstType>, Diagnostic> Parser::ParseTypeAnn()
{
  Ptr<type::Type> type;
  switch (m_CurrToken.m_Type)
  {
  case TokenType::I8:
    type = MakePtr<type::Type>(type::Base::I8);
    break;
  case TokenType::I16:
    type = MakePtr<type::Type>(type::Base::I16);
    break;
  case TokenType::I32:
    type = MakePtr<type::Type>(type::Base::I32);
    break;
  case TokenType::I64:
    type = MakePtr<type::Type>(type::Base::I64);
    break;
  case TokenType::U8:
    type = MakePtr<type::Type>(type::Base::U8);
    break;
  case TokenType::U16:
    type = MakePtr<type::Type>(type::Base::U16);
    break;
  case TokenType::U32:
    type = MakePtr<type::Type>(type::Base::U32);
    break;
  case TokenType::U64:
    type = MakePtr<type::Type>(type::Base::U32);
    break;
  case TokenType::Float:
    type = MakePtr<type::Type>(type::Base::Float);
    break;
  case TokenType::Void:
    type = MakePtr<type::Type>(type::Base::VOID);
    break;
  case TokenType::String:
    type = MakePtr<type::Type>(type::Base::STRING);
    break;
  case TokenType::Fun:
    return ParseFunTypeAnn();
  default:
    return Diagnostic(Errno::SYNTAX_ERROR, m_CurrToken.m_Position, m_ModuleID, DiagnosticSeverity::ERROR, "expect type annotation, try 'i32', 'string', ...");
  }
  return MakePtr(AstType(Next().unwrap(), type));
}

Result<Ptr<AstType>, Diagnostic> Parser::ParseFunTypeAnn()
{
  Position position = Expect(TokenType::Fun).unwrap();
  Expect(TokenType::Lparen).unwrap();
  std::vector<Ptr<type::Type>> argsTypes;
  while (!IsEof() && TokenType::Rparen != m_CurrToken.m_Type)
  {
    auto typeRes = ParseTypeAnn();
    if (typeRes.is_err())
    {
      return typeRes.unwrap_err();
    }
    argsTypes.push_back(typeRes.unwrap()->GetType());
  }
  Expect(TokenType::Rparen).unwrap();
  Expect(TokenType::Arrow).unwrap();
  auto returnType = ParseTypeAnn().unwrap();
  position.m_End = returnType->GetPos().m_End;
  size_t argsCount = argsTypes.size();
  auto functionType = MakePtr(type::Function(argsCount, std::move(argsTypes), returnType->GetType()));
  return MakePtr(AstType(position, functionType));
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
  case TokenType::Fun:
  case TokenType::Class:
  case TokenType::Let:
    return true;
  default:
    return false;
  }
}

bool Parser::IsEof()
{
  return TokenType::END == m_CurrToken.m_Type;
}
