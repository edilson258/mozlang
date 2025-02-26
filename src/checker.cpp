#include <cassert>
#include <format>
#include <memory>
#include <optional>
#include <vector>

#include "ast.h"
#include "checker.h"
#include "context.h"
#include "diagnostic.h"
#include "error.h"
#include "token.h"
#include "type.h"

std::vector<Diagnostic> Checker::check()
{
  EnterScope(ScopeType::GLOBAL);

  auto printlnArgs = {std::make_shared<Type>(BaseType::F_STRING)};
  auto printlnType = std::make_shared<FunctionType>(0, std::move(printlnArgs), true);
  auto println_bind = std::make_shared<BindingFunction>(Position(), Position(), Position(), printlnType, 0, true);
  SaveBind("println", println_bind);

  for (auto statement : Ast.Program)
  {
    CheckStatement(statement);
  }

  LeaveScope();

  return std::move(Diagnostics);
}

std::optional<std::shared_ptr<Binding>> Checker::CheckStatement(std::shared_ptr<Statement> statement)
{
  switch (statement.get()->Type)
  {
  case StatementType::BLOCK:
    return CheckStatementBlock(std::static_pointer_cast<StatementBlock>(statement));
  case StatementType::RETURN:
    return CheckStatementReturn(std::static_pointer_cast<StatementReturn>(statement));
  case StatementType::FUNCTION:
    return CheckStatementFunction(std::static_pointer_cast<StatementFunction>(statement));
  case StatementType::EXPRESSION:
    return CheckExpression(std::static_pointer_cast<Expression>(statement));
  }
  return std::nullopt;
}

std::optional<std::shared_ptr<Binding>> Checker::CheckStatementFunction(std::shared_ptr<StatementFunction> functionStatement)
{
  auto bindWithSameName = Scopes.back().Ctx.Get(functionStatement.get()->Name.get()->Value);
  if (bindWithSameName.has_value())
  {
    Diagnostics.push_back(Diagnostic(Errno::NAME_ERROR, functionStatement.get()->Name.get()->Pos, Sourc, DiagnosticSeverity::ERROR, "name already in use"));
    return std::nullopt;
  }

  std::vector<std::shared_ptr<Type>> functionArgsTypes = {};
  for (auto &functionParam : functionStatement.get()->Params.Params)
  {
    if (!functionParam.Typ.has_value())
    {
      functionArgsTypes.push_back(std::make_shared<Type>(BaseType::ANY));
      continue;
    }
    assert(false);
  }

  auto functionType = std::make_shared<FunctionType>(FunctionType(functionStatement.get()->Params.Params.size(), std::move(functionArgsTypes)));
  auto functionBind = std::make_shared<BindingFunction>(functionStatement.get()->Pos, functionStatement.get()->Name.get()->Pos, functionStatement.get()->Params.Pos, functionType, 0);
  SaveBind(functionStatement.get()->Name.get()->Value, functionBind);

  EnterScope(ScopeType::FUNCTION);

  for (auto &param : functionStatement.get()->Params.Params)
  {
    // TODO: check for duplicated param names
    auto paramType = std::make_shared<Type>(BaseType::ANY);
    auto paramBind = std::make_shared<BindingParameter>(param.Pos, paramType, 0);
    SaveBind(param.Name.get()->Value, paramBind);
  }

  CheckStatementBlock(functionStatement.get()->Body);

  LeaveScope();

  return std::nullopt;
}

std::optional<std::shared_ptr<Binding>> Checker::CheckStatementReturn(std::shared_ptr<StatementReturn> returnStatement)
{
  if (!IsWithinScope(ScopeType::FUNCTION))
  {
    Diagnostics.push_back(Diagnostic(Errno::SYNTAX_ERROR, returnStatement.get()->Pos, Sourc, DiagnosticSeverity::ERROR, "cannot return from outside of a function"));
    return std::nullopt;
  }
  if (returnStatement.get()->Value.has_value())
  {
    auto x = CheckExpression(returnStatement.get()->Value.value());
    if (x.has_value())
    {
    }
  }
  return std::nullopt;
}

std::optional<std::shared_ptr<Binding>> Checker::CheckStatementBlock(std::shared_ptr<StatementBlock> blockStatement)
{
  for (auto &statement : blockStatement.get()->Stmts)
  {
    CheckStatement(statement);
  }
  return std::nullopt;
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
  auto calleeBindOpt = CheckExpression(callExpression.get()->Callee);
  if (!calleeBindOpt.has_value())
  {
    return std::nullopt;
  }

  auto calleeBind = calleeBindOpt.value();
  if (BindingType::FUNCTION != calleeBind.get()->Typ)
  {
    Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, callExpression.get()->Callee.get()->Pos, Sourc, DiagnosticSeverity::ERROR, "call to non-callable object"));
    return std::nullopt;
  }

  auto calleeBindFunction = std::static_pointer_cast<BindingFunction>(calleeBind);
  auto callee = calleeBindFunction.get()->FnType;

  if (callee->RequiredArgumentsCount != callExpression.get()->Arguments.size() && (!callee.get()->IsVariadicArguments || (callee.get()->RequiredArgumentsCount && callExpression.get()->Arguments.size() < callee.get()->RequiredArgumentsCount)))
  {
    Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, callExpression.get()->ArgumentsPosition, Sourc, DiagnosticSeverity::ERROR, std::format("missing required arguments, expect {} but got {}", callee.get()->RequiredArgumentsCount, callExpression.get()->Arguments.size())));
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
    x.value().get()->Used = true;
    return x.value();
  }
  Diagnostics.push_back(Diagnostic(Errno::NAME_ERROR, identifierExpression.get()->Pos, Sourc, DiagnosticSeverity::ERROR, std::format("undefined name: '{}'", identifierExpression.get()->Value)));
  return std::nullopt;
}

std::optional<std::shared_ptr<Binding>> Checker::CheckExpressionString(std::shared_ptr<ExpressionString> stringExpression)
{
  auto stringLiteralBind = std::make_shared<BindingLiteral>(stringExpression.get()->Pos, BaseType::STRING, 0);
  SaveBind(GetTmpName(), stringLiteralBind);
  return stringLiteralBind;
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

    switch (binding.second.get()->Typ)
    {
    case BindingType::PARAMETER:
      Diagnostics.push_back(Diagnostic(Errno::UNUSED_VALUE, binding.second.get()->Pos, Sourc, DiagnosticSeverity::WARN, "unused parameter"));
      break;
    case BindingType::LITERAL:
      // auto literalBinding = std::static_pointer_cast<BindingLiteral>(binding.second);
      Diagnostics.push_back(Diagnostic(Errno::UNUSED_VALUE, binding.second.get()->Pos, Sourc, DiagnosticSeverity::WARN, "expression results to unused value"));
      break;
    case BindingType::FUNCTION:
    {
      auto functionBinding = std::static_pointer_cast<BindingFunction>(binding.second);
      Diagnostics.push_back(Diagnostic(Errno::UNUSED_VALUE, functionBinding.get()->NamePosition, Sourc, DiagnosticSeverity::WARN, std::format("function '{}' never gets called", binding.first)));
    }
    break;
    }
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

bool Checker::IsWithinScope(ScopeType scopeType)
{
  for (auto it = Scopes.rbegin(); it != Scopes.rend(); ++it)
  {
    if (it->Type == scopeType)
    {
      return true;
    }
  }
  return false;
}
