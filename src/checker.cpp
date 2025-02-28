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

  auto printlnArgs = {std::make_shared<type::Type>(type::Base::F_STRING)};
  auto printlnType = std::make_shared<type::Function>(0, std::move(printlnArgs), true);
  auto printlnBind = std::make_shared<BindingFunction>(Position(), Position(), Position(), printlnType, 0, true);
  SaveBind("println", printlnBind);

  for (auto statement : m_Ast.m_Program)
  {
    CheckStatement(statement);
  }

  LeaveScope();

  return std::move(m_Diagnostics);
}

std::optional<std::shared_ptr<Binding>> Checker::CheckStatement(std::shared_ptr<Statement> statement)
{
  switch (statement.get()->m_Type)
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
  auto bindWithSameName = m_Scopes.back().m_Context.Get(functionStatement.get()->m_Identifier.get()->m_Value);
  if (bindWithSameName.has_value())
  {
    m_Diagnostics.push_back(Diagnostic(Errno::NAME_ERROR, functionStatement.get()->m_Identifier.get()->m_Position, m_Source, DiagnosticSeverity::ERROR, "name already in use"));
    return std::nullopt;
  }

  std::vector<std::shared_ptr<type::Type>> functionArgsTypes = {};
  for (auto &param : functionStatement.get()->m_Params.m_Params)
  {
    functionArgsTypes.push_back(param.m_Type);
  }

  auto functionType = std::make_shared<type::Function>(type::Function(functionArgsTypes.size(), std::move(functionArgsTypes)));
  auto functionBind = std::make_shared<BindingFunction>(functionStatement.get()->m_Position, functionStatement.get()->m_Identifier.get()->m_Position, functionStatement.get()->m_Params.m_Position, functionType, 0);
  SaveBind(functionStatement.get()->m_Identifier.get()->m_Value, functionBind);

  EnterScope(ScopeType::FUNCTION);

  for (auto &param : functionStatement.get()->m_Params.m_Params)
  {
    // TODO: check for duplicated param names
    SaveBind(param.m_Identifier.get()->m_Value, std::make_shared<Binding>(Binding(BindType::PARAMETER, param.m_Type, m_Source->m_ID, param.m_Position)));
  }

  CheckStatementBlock(functionStatement.get()->m_Body);

  LeaveScope();

  return std::nullopt;
}

std::optional<std::shared_ptr<Binding>> Checker::CheckStatementReturn(std::shared_ptr<StatementReturn> returnStatement)
{
  if (!IsWithinScope(ScopeType::FUNCTION))
  {
    m_Diagnostics.push_back(Diagnostic(Errno::SYNTAX_ERROR, returnStatement.get()->m_Position, m_Source, DiagnosticSeverity::ERROR, "cannot return from outside of a function"));
    return std::nullopt;
  }
  if (returnStatement.get()->m_Value.has_value())
  {
    auto x = CheckExpression(returnStatement.get()->m_Value.value());
    if (x.has_value())
    {
    }
  }
  return std::nullopt;
}

std::optional<std::shared_ptr<Binding>> Checker::CheckStatementBlock(std::shared_ptr<StatementBlock> blockStatement)
{
  for (auto &statement : blockStatement.get()->m_Stmts)
  {
    CheckStatement(statement);
  }
  return std::nullopt;
}

std::optional<std::shared_ptr<Binding>> Checker::CheckExpression(std::shared_ptr<Expression> expression)
{
  switch (expression.get()->m_Type)
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
  auto calleeBindOpt = CheckExpression(callExpression.get()->m_Callee);
  if (!calleeBindOpt.has_value())
  {
    return std::nullopt;
  }

  auto calleeBind = calleeBindOpt.value();
  if (BindType::FUNCTION != calleeBind.get()->m_BindType)
  {
    m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, callExpression.get()->m_Callee.get()->m_Position, m_Source, DiagnosticSeverity::ERROR, "call to non-callable object"));
    return std::nullopt;
  }

  auto callee = std::static_pointer_cast<type::Function>(calleeBind->m_Type);

  if (callee.get()->m_ReqArgsCount != callExpression.get()->m_Arguments.size() && (!callee.get()->m_IsVariadicArguments || (callee.get()->m_ReqArgsCount && callExpression.get()->m_Arguments.size() < callee.get()->m_ReqArgsCount)))
  {
    m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, callExpression.get()->m_ArgumentsPosition, m_Source, DiagnosticSeverity::ERROR, std::format("missing required arguments, expect {} but got {}", callee.get()->m_ReqArgsCount, callExpression.get()->m_Arguments.size())));
  }

  for (auto argument : callExpression.get()->m_Arguments)
  {
    auto argumentBind = CheckExpression(argument);
    if (!argumentBind.has_value())
    {
      continue;
    }
    argumentBind.value().get()->m_IsUsed = true;
  }
  return std::nullopt;
}

std::optional<std::shared_ptr<Binding>> Checker::CheckExpressionIdentifier(std::shared_ptr<ExpressionIdentifier> identifierExpression)
{
  auto x = LookupBind(identifierExpression.get()->m_Value);
  if (x.has_value())
  {
    x.value().get()->m_IsUsed = true;
    return x.value();
  }
  m_Diagnostics.push_back(Diagnostic(Errno::NAME_ERROR, identifierExpression.get()->m_Position, m_Source, DiagnosticSeverity::ERROR, std::format("undefined name: '{}'", identifierExpression.get()->m_Value)));
  return std::nullopt;
}

std::optional<std::shared_ptr<Binding>> Checker::CheckExpressionString(std::shared_ptr<ExpressionString> stringExpression)
{
  auto stringLiteralBind = std::make_shared<Binding>(Binding(BindType::LITERAL, std::make_shared<type::Type>(type::Type(type::Base::STRING)), 0, stringExpression.get()->m_Position));
  SaveBind(GetTmpName(), stringLiteralBind);
  return stringLiteralBind;
}

void Checker::EnterScope(ScopeType scopeType)
{
  m_Scopes.push_back(Scope(scopeType));
}

void Checker::LeaveScope()
{
  for (auto &binding : m_Scopes.back().m_Context.Store)
  {
    if (binding.second.get()->m_IsUsed || binding.first.starts_with('_'))
    {
      continue;
    }

    switch (binding.second.get()->m_BindType)
    {
    case BindType::PARAMETER:
      m_Diagnostics.push_back(Diagnostic(Errno::UNUSED_VALUE, binding.second.get()->m_Position, m_Source, DiagnosticSeverity::WARN, "unused parameter"));
      break;
    case BindType::LITERAL:
      // auto literalBinding = std::static_pointer_cast<BindingLiteral>(binding.second);
      m_Diagnostics.push_back(Diagnostic(Errno::UNUSED_VALUE, binding.second.get()->m_Position, m_Source, DiagnosticSeverity::WARN, "expression results to unused value"));
      break;
    case BindType::FUNCTION:
    {
      auto functionBinding = std::static_pointer_cast<BindingFunction>(binding.second);
      m_Diagnostics.push_back(Diagnostic(Errno::UNUSED_VALUE, functionBinding.get()->NamePosition, m_Source, DiagnosticSeverity::WARN, std::format("function '{}' never gets called", binding.first)));
    }
    break;
    }
  }
  m_Scopes.pop_back();
}

std::optional<std::shared_ptr<Binding>> Checker::LookupBind(std::string name)
{
  for (auto it = m_Scopes.rbegin(); it != m_Scopes.rend(); ++it)
  {
    auto binding = it->m_Context.Get(name);
    if (binding)
      return binding;
  }
  return std::nullopt;
}

void Checker::SaveBind(std::string name, std::shared_ptr<Binding> bind)
{
  m_Scopes.back().m_Context.Save(name, bind);
}

bool Checker::IsWithinScope(ScopeType scopeType)
{
  for (auto it = m_Scopes.rbegin(); it != m_Scopes.rend(); ++it)
  {
    if (it->m_Type == scopeType)
    {
      return true;
    }
  }
  return false;
}
