#pragma once

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
  Parser(Module *module, ModuleManager &modManager) : m_Module(module), m_ModuleID(module->m_ID), m_Lexer(Lexer(module->m_ID, modManager)), m_CurrToken(), m_NextToken() {};

  std::optional<Diagnostic> Parse();

private:
  Module *m_Module;
  ModuleID m_ModuleID;
  Lexer m_Lexer;
  Token m_CurrToken;
  Token m_NextToken;
  bool m_HasPubModifier;

  bool IsEof();
  Result<Position, Diagnostic> Next();
  Result<Position, Diagnostic> Expect(TokenType);
  bool AcceptsPubModifier(TokenType);
  bool EraseIfPubModifier();

  Result<Stmt *, Diagnostic> ParseStmt();
  Result<Stmt *, Diagnostic> ParseStmtImport();
  Result<Stmt *, Diagnostic> ParseStmtFunction();
  Result<Stmt *, Diagnostic> ParseStmtReturn();
  Result<Stmt *, Diagnostic> ParseStmtLet();
  Result<Stmt *, Diagnostic> ParseStmtBlock();
  Result<Stmt *, Diagnostic> ParseStmtExpr();
  Result<Expr *, Diagnostic> ParseExpr(Prec);
  Result<Expr *, Diagnostic> ParseExprPrim();
  Result<IdentExpr *, Diagnostic> ParseExprIdent();
  Result<CallExpr *, Diagnostic> ParseExprCall(Expr *);
  Result<AssignExpr *, Diagnostic> ParseExprAssign(Expr *);
  Result<FieldAccExpr *, Diagnostic> ParseExprFieldAcc(Expr *);

  Result<bool, Diagnostic> ParsePubAccMod();
  Result<FunParams, Diagnostic> ParseFunParams();
  Result<AstType *, Diagnostic> ParseTypeAnn();
  Result<AstType *, Diagnostic> ParseFunTypeAnn();
};
