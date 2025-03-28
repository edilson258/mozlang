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
  Checker(Ptr<Module> module, ModuleManager &modManager) : m_Module(module), m_ModManager(modManager), m_Scopes(), m_Diagnostics() {};

  std::vector<Diagnostic> Check();

private:
  Ptr<Module> m_Module;
  ModuleManager &m_ModManager;
  std::vector<Scope> m_Scopes;
  std::vector<Diagnostic> m_Diagnostics;

  void EnterScope(ScopeType);
  void LeaveScope();
  Ptr<Bind> LookupBind(std::string name);
  void SaveBind(std::string name, Ptr<Bind> bind);
  bool IsWithinScope(ScopeType);

  // utils
  std::string NormalizeImportPath(bool, std::vector<Ptr<IdentExpr>>);

  Ptr<Bind> CheckStmt(Ptr<Stmt>);
  Ptr<Bind> CheckStmtFun(Ptr<FunStmt>);
  Ptr<Bind> CheckStmtRet(Ptr<RetStmt>);
  Ptr<Bind> CheckStmtBlock(Ptr<BlockStmt>);
  Ptr<Bind> CheckStmtLet(Ptr<LetStmt>);
  Ptr<Bind> CheckStmtImport(Ptr<ImportStmt>);
  Ptr<Bind> CheckExpr(Ptr<Expr>);
  Ptr<Bind> CheckExprCall(Ptr<CallExpr>);
  Ptr<Bind> CheckExprString(Ptr<StringExpr>);
  Ptr<Bind> CheckExprIdent(Ptr<IdentExpr>);
  Ptr<Bind> CheckExprAssign(Ptr<AssignExpr>);
  Ptr<Bind> CheckExprFieldAcc(Ptr<FieldAccExpr>);
};
