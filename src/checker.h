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
  ScopeType Type;
  Context Ctx;

  Scope(ScopeType type) : Type(type), Ctx() {};
};

class Checker
{
public:
  Checker(AST &ast, std::shared_ptr<Source> source) : Ast(ast), Sourc(source), Scopes(), Diagnostics() {};

  std::vector<Diagnostic> check();

private:
  AST &Ast;
  std::shared_ptr<Source> Sourc;
  std::vector<Scope> Scopes;
  std::vector<Diagnostic> Diagnostics;

  void EnterScope(ScopeType scopeType);
  void LeaveScope();
  std::optional<std::shared_ptr<Binding>> LookupBind(std::string name);
  void SaveBind(std::string name, std::shared_ptr<Binding> bind);

  std::string GetTmpName()
  {
    static int counter = 0;
    return std::string("t___m___p___n___a___m___e___") + std::to_string(counter++);
  }

  bool IsWithinScope(ScopeType);

  std::optional<std::shared_ptr<Binding>> CheckStatement(std::shared_ptr<Statement>);
  std::optional<std::shared_ptr<Binding>> CheckStatementFunction(std::shared_ptr<StatementFunction>);
  std::optional<std::shared_ptr<Binding>> CheckStatementReturn(std::shared_ptr<StatementReturn>);
  std::optional<std::shared_ptr<Binding>> CheckStatementBlock(std::shared_ptr<StatementBlock>);
  std::optional<std::shared_ptr<Binding>> CheckExpression(std::shared_ptr<Expression>);
  std::optional<std::shared_ptr<Binding>> CheckExpressionCall(std::shared_ptr<ExpressionCall>);
  std::optional<std::shared_ptr<Binding>> CheckExpressionString(std::shared_ptr<ExpressionString>);
  std::optional<std::shared_ptr<Binding>> CheckExpressionIdentifier(std::shared_ptr<ExpressionIdentifier>);
};
