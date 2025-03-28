#pragma once

#include <string>
#include <vector>

#include "ast.h"
#include "context.h"
#include "diagnostic.h"
#include "module.h"

enum class ScopeType
{
  GLOBAL,
  FUNCTION,
};

class Scope
{
public:
  ScopeType m_Type;
  ModuleContext m_Context;

  Scope(ScopeType type) : m_Type(type), m_Context() {};
};

class Checker
{
public:
  Checker(Module *module, ModuleManager &modManager) : m_Module(module), m_ModManager(modManager), m_Scopes(), m_Diagnostics() {};

  std::vector<Diagnostic> Check();

private:
  Module *m_Module;
  ModuleManager &m_ModManager;
  std::vector<Scope> m_Scopes;
  std::vector<Diagnostic> m_Diagnostics;

  void EnterScope(ScopeType);
  void LeaveScope();
  Bind *LookupBind(std::string name);
  void SaveBind(std::string name, Bind *bind);
  bool IsWithinScope(ScopeType);

  // utils
  std::string NormalizeImportPath(bool, std::vector<IdentExpr *>);

  Bind *CheckStmt(Stmt *);
  Bind *CheckStmtFun(FunStmt *);
  Bind *CheckStmtRet(RetStmt *);
  Bind *CheckStmtBlock(BlockStmt *);
  Bind *CheckStmtLet(LetStmt *);
  Bind *CheckStmtImport(ImportStmt *);
  Bind *CheckExpr(Expr *);
  Bind *CheckExprCall(CallExpr *);
  Bind *CheckExprString(StringExpr *);
  Bind *CheckExprIdent(IdentExpr *);
  Bind *CheckExprAssign(AssignExpr *);
  Bind *CheckExprFieldAcc(FieldAccExpr *);
};
