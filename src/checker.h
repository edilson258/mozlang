#pragma once

#include <memory>
#include <optional>
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
  Checker(std::shared_ptr<Module> module, ModuleManager &modManager) : m_Module(module), m_ModManager(modManager), m_Scopes(), m_Diagnostics() {};

  std::vector<Diagnostic> Check();

private:
  std::shared_ptr<Module> m_Module;
  ModuleManager &m_ModManager;
  std::vector<Scope> m_Scopes;
  std::vector<Diagnostic> m_Diagnostics;

  void EnterScope(ScopeType);
  void LeaveScope();
  std::optional<std::shared_ptr<Binding>> LookupBind(std::string name);
  void SaveBind(std::string name, std::shared_ptr<Binding> bind);
  bool IsWithinScope(ScopeType);

  // utils
  std::string NormalizeImportPath(bool, std::vector<std::shared_ptr<IdentExpr>>);

  std::optional<std::shared_ptr<Binding>> CheckStmt(std::shared_ptr<Stmt>);
  std::optional<std::shared_ptr<Binding>> CheckStmtFun(std::shared_ptr<FunStmt>);
  std::optional<std::shared_ptr<Binding>> CheckStmtFunSign(std::shared_ptr<FunSignStmt>);
  std::optional<std::shared_ptr<Binding>> CheckStmtRet(std::shared_ptr<RetStmt>);
  std::optional<std::shared_ptr<Binding>> CheckStmtBlock(std::shared_ptr<BlockStmt>);
  std::optional<std::shared_ptr<Binding>> CheckStmtLet(std::shared_ptr<LetStmt>);
  std::optional<std::shared_ptr<Binding>> CheckStmtImport(std::shared_ptr<ImportStmt>);
  std::optional<std::shared_ptr<Binding>> CheckExpr(std::shared_ptr<Expr>);
  std::optional<std::shared_ptr<Binding>> CheckExprCall(std::shared_ptr<CallExpr>);
  std::optional<std::shared_ptr<Binding>> CheckExprString(std::shared_ptr<StringExpr>);
  std::optional<std::shared_ptr<Binding>> CheckExprIdent(std::shared_ptr<IdentExpr>);
  std::optional<std::shared_ptr<Binding>> CheckExprAssign(std::shared_ptr<AssignExpr>);
  std::optional<std::shared_ptr<Binding>> CheckExprFieldAcc(std::shared_ptr<FieldAccExpr>);
};
