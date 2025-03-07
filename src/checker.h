#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ast.h"
#include "context.h"
#include "diagnostic.h"
#include "loader.h"

enum class ScopeType
{
  GLOBAL,
  FUNCTION,
};

class Scope
{
public:
  ScopeType m_Type;
  Context m_Context;

  Scope(ScopeType type) : m_Type(type), m_Context() {};
};

class Checker
{
public:
  Checker(AST &ast, ModuleID moduleID, ModuleManager &modManager, std::shared_ptr<Context> outContext) : m_Ast(ast), m_ModuleID(moduleID), m_ModManager(modManager), m_OutContext(outContext), m_Scopes(), m_Diagnostics() {};

  std::vector<Diagnostic> Check();

private:
  AST &m_Ast;
  ModuleID m_ModuleID;
  ModuleManager &m_ModManager;
  std::shared_ptr<Context> m_OutContext;
  std::vector<Scope> m_Scopes;
  std::vector<Diagnostic> m_Diagnostics;

  void EnterScope(ScopeType scopeType);
  void LeaveScope();
  std::optional<std::shared_ptr<Binding>> LookupBind(std::string name);
  void SaveBind(std::string name, std::shared_ptr<Binding> bind);

  bool IsWithinScope(ScopeType);

  std::optional<std::shared_ptr<Binding>> CheckStatement(std::shared_ptr<Statement>);
  std::optional<std::shared_ptr<Binding>> CheckStatementFunction(std::shared_ptr<StatementFunction>);
  std::optional<std::shared_ptr<Binding>> CheckStatementReturn(std::shared_ptr<StatementReturn>);
  std::optional<std::shared_ptr<Binding>> CheckStatementBlock(std::shared_ptr<StatementBlock>);
  std::optional<std::shared_ptr<Binding>> CheckStatementLet(std::shared_ptr<StatementLet>);
  std::optional<std::shared_ptr<Binding>> CheckStatementImport(std::shared_ptr<StatementImport>);
  std::optional<std::shared_ptr<Binding>> CheckExpression(std::shared_ptr<Expression>);
  std::optional<std::shared_ptr<Binding>> CheckExpressionCall(std::shared_ptr<ExpressionCall>);
  std::optional<std::shared_ptr<Binding>> CheckExpressionString(std::shared_ptr<ExpressionString>);
  std::optional<std::shared_ptr<Binding>> CheckExpressionIdentifier(std::shared_ptr<ExpressionIdentifier>);
  std::optional<std::shared_ptr<Binding>> CheckExpressionAssign(std::shared_ptr<ExpressionAssign>);
  std::optional<std::shared_ptr<Binding>> CheckExpressionFieldAccess(std::shared_ptr<ExpressionFieldAccess>);
};
