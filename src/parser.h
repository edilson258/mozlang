#pragma once

#include <memory>
#include <optional>

#include "ast.h"
#include "diagnostic.h"
#include "lexer.h"
#include "module.h"
#include "result.h"
#include "token.h"

class Parser
{
public:
  Parser(std::shared_ptr<Module> module, ModuleManager &modManager) : m_Module(module), m_ModuleID(module.get()->m_ID), m_Lexer(Lexer(module.get()->m_ID, modManager)), m_CurrToken(), m_NextToken() {};

  std::optional<Diagnostic> Parse();

private:
  std::shared_ptr<Module> m_Module;
  ModuleID m_ModuleID;
  Lexer m_Lexer;
  Token m_CurrToken;
  Token m_NextToken;
  std::optional<Token> m_PubAccModToken;

  bool IsEof();
  Result<Position, Diagnostic> Next();
  Result<Position, Diagnostic> Expect(TokenType);
  bool AcceptsPubAccMod(TokenType);
  std::optional<Token> GetAndErasePubAccMod();

  Result<std::shared_ptr<Stmt>, Diagnostic> ParseStmt();
  Result<std::shared_ptr<Stmt>, Diagnostic> ParseStmtImport();
  Result<std::shared_ptr<Stmt>, Diagnostic> ParseStmtFunction();
  Result<std::shared_ptr<Stmt>, Diagnostic> ParseStmtReturn();
  Result<std::shared_ptr<Stmt>, Diagnostic> ParseStmtLet();
  Result<std::shared_ptr<Stmt>, Diagnostic> ParseStmtBlock();
  Result<std::shared_ptr<Stmt>, Diagnostic> ParseStmtExpr();
  Result<std::shared_ptr<Expr>, Diagnostic> ParseExpr(Prec);
  Result<std::shared_ptr<Expr>, Diagnostic> ParseExprPrim();
  Result<std::shared_ptr<IdentExpr>, Diagnostic> ParseExprIdent();
  Result<std::shared_ptr<CallExpr>, Diagnostic> ParseExprCall(std::shared_ptr<Expr>);
  Result<std::shared_ptr<AssignExpr>, Diagnostic> ParseExprAssign(std::shared_ptr<Expr>);
  Result<std::shared_ptr<FieldAccExpr>, Diagnostic> ParseExprFieldAcc(std::shared_ptr<Expr>);

  std::optional<Diagnostic> ParsePubAccMod();
  Result<FunParams, Diagnostic> ParseFunParams();
  Result<AstType, Diagnostic> ParseTypeAnn();
  Result<AstType, Diagnostic> ParseFunTypeAnn();
};
