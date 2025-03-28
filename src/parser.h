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
  Parser(Ptr<Module> module, ModuleManager &modManager) : m_Module(module), m_ModuleID(module->m_ID), m_Lexer(Lexer(module->m_ID, modManager)), m_CurrToken(), m_NextToken(), m_HasPubModifier(false) {};

  std::optional<Diagnostic> Parse();

private:
  Ptr<Module> m_Module;
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

  Result<Ptr<Stmt>, Diagnostic> ParseStmt();
  Result<Ptr<Stmt>, Diagnostic> ParseStmtImport();
  Result<Ptr<Stmt>, Diagnostic> ParseStmtFunction();
  Result<Ptr<Stmt>, Diagnostic> ParseStmtReturn();
  Result<Ptr<Stmt>, Diagnostic> ParseStmtLet();
  Result<Ptr<Stmt>, Diagnostic> ParseStmtBlock();
  Result<Ptr<Stmt>, Diagnostic> ParseStmtExpr();
  Result<Ptr<Expr>, Diagnostic> ParseExpr(Prec);
  Result<Ptr<Expr>, Diagnostic> ParseExprPrim();
  Result<Ptr<IdentExpr>, Diagnostic> ParseExprIdent();
  Result<Ptr<CallExpr>, Diagnostic> ParseExprCall(Ptr<Expr>);
  Result<Ptr<AssignExpr>, Diagnostic> ParseExprAssign(Ptr<Expr>);
  Result<Ptr<FieldAccExpr>, Diagnostic> ParseExprFieldAcc(Ptr<Expr>);

  Result<bool, Diagnostic> ParsePubAccMod();
  Result<FunParams, Diagnostic> ParseFunParams();
  Result<Ptr<AstType>, Diagnostic> ParseTypeAnn();
  Result<Ptr<AstType>, Diagnostic> ParseFunTypeAnn();
};
