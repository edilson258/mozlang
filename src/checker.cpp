#include <format>
#include <memory>
#include <optional>
#include <vector>

#include "ast.h"
#include "checker.h"
#include "context.h"
#include "diagnostic.h"
#include "error.h"
#include "type.h"

std::vector<Diagnostic> Checker::check()
{
  EnterScope(ScopeType::GLOBAL);

  std::vector<std::shared_ptr<Type>> println_args = {std::make_shared<Type>(BaseType::F_STRING)};
  std::shared_ptr<FunctionType> println_t = std::make_shared<FunctionType>(0, std::move(println_args), true);
  std::shared_ptr<Binding> println_bind = std::make_shared<Binding>(println_t, Position(), BindingOrigin::BUILTIN, true);
  SaveBind("println", println_bind);

  for (auto statement : Ast.Program)
  {
    CheckStatement(statement);
  }

  LeaveScope();

  return Diagnostics;
}

void Checker::CheckStatement(std::shared_ptr<Statement> statement)
{
  switch (statement.get()->Type)
  {
  case StatementType::EXPRESSION:
  {
    auto t = CheckExpression(std::static_pointer_cast<Expression>(statement));
    break;
  }
  }
}

std::optional<std::shared_ptr<Binding>> Checker::CheckExpression(std::shared_ptr<Expression> expression)
{
  switch (expression.get()->Type)
  {
  case ExpressionType::CALL:
    return CheckExpressionCall(std::static_pointer_cast<ExpressionCall>(expression));
  case ExpressionType::STRING:
    return CheckExpressionString(std::static_pointer_cast<ExpressionString>(expression));
  case ExpressionType::IDENTIFIER:
    return CheckExpressionIdentifier(std::static_pointer_cast<ExpressionIdentifier>(expression));
  }
  return std::nullopt;
}

std::optional<std::shared_ptr<Binding>> Checker::CheckExpressionCall(std::shared_ptr<ExpressionCall> callExpression)
{
  auto calleeBind = CheckExpression(callExpression.get()->Callee);
  if (!calleeBind.has_value())
  {
    return std::nullopt;
  }

  std::shared_ptr<Type> calleeType = calleeBind.value().get()->Typ;

  if (BaseType::FUNCTION != calleeType.get()->Base)
  {
    // TODO: display expression
    Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, callExpression.get()->Callee.get()->Pos, Sourc, DiagnosticSeverity::ERROR, "call to non-callable object"));
    return std::nullopt;
  }

  std::shared_ptr<FunctionType> Callee = std::static_pointer_cast<FunctionType>(calleeType);

  if (Callee->RequiredArgumentsCount != callExpression.get()->Arguments.size() && (!Callee.get()->IsVariadicArguments || (Callee.get()->RequiredArgumentsCount && callExpression.get()->Arguments.size() < Callee.get()->RequiredArgumentsCount)))
  {
    Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, callExpression.get()->ArgumentsPosition, Sourc, DiagnosticSeverity::ERROR, std::format("missing required arguments, expect {} but got {}", Callee.get()->RequiredArgumentsCount, callExpression.get()->Arguments.size())));
  }

  for (auto argument : callExpression.get()->Arguments)
  {
    auto argumentBind = CheckExpression(argument);
    if (!argumentBind.has_value())
    {
      continue;
    }
    argumentBind.value().get()->Used = true;
  }
  return std::nullopt;
}

std::optional<std::shared_ptr<Binding>> Checker::CheckExpressionIdentifier(std::shared_ptr<ExpressionIdentifier> identifierExpression)
{
  auto x = LookupBind(identifierExpression.get()->Value);
  if (x.has_value())
  {
    return x.value();
  }
  Diagnostics.push_back(Diagnostic(Errno::NAME_ERROR, identifierExpression.get()->Pos, Sourc, DiagnosticSeverity::ERROR, std::format("undefined name: '{}'", identifierExpression.get()->Value)));
  return std::nullopt;
}

std::optional<std::shared_ptr<Binding>> Checker::CheckExpressionString(std::shared_ptr<ExpressionString> stringExpression)
{
  auto bind = Binding::make_string_literal(stringExpression.get()->Pos);
  SaveBind(GetTmpName(), bind);
  return bind;
}

void Checker::EnterScope(ScopeType scopeType)
{
  Scopes.push_back(Scope(scopeType));
}

void Checker::LeaveScope()
{
  for (auto &binding : Scopes.back().Ctx.Store)
  {
    if (binding.second.get()->Used || binding.first.starts_with('_'))
    {
      continue;
    }
    Diagnostics.push_back(Diagnostic(Errno::UNUSED_VALUE, binding.second.get()->Pos, Sourc, DiagnosticSeverity::WARN, "this expression results to unused value"));
  }
  Scopes.pop_back();
}

std::optional<std::shared_ptr<Binding>> Checker::LookupBind(std::string name)
{
  for (auto it = Scopes.rbegin(); it != Scopes.rend(); ++it)
  {
    auto binding = it->Ctx.Get(name);
    if (binding)
      return binding;
  }
  return std::nullopt;
}

void Checker::SaveBind(std::string name, std::shared_ptr<Binding> bind)
{
  Scopes.back().Ctx.Save(name, bind);
}
