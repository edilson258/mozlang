#include <cstddef>
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
  auto printlnType = std::make_shared<type::Function>(type::Function(0, std::move(printlnArgs), std::make_shared<type::Type>(type::Base::VOID), true));
  // TODO: module ID
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
    m_Diagnostics.push_back(Diagnostic(Errno::NAME_ERROR, functionStatement.get()->m_Identifier.get()->m_Position, m_ModuleID, DiagnosticSeverity::ERROR, "name already in use"));
    return std::nullopt;
  }

  std::vector<std::shared_ptr<type::Type>> functionArgsTypes;
  for (auto &param : functionStatement.get()->m_Params.m_Params)
  {
    functionArgsTypes.push_back(param.m_TypeAnnotation.m_ReturnType);
  }
  auto functionType = std::make_shared<type::Function>(type::Function(functionStatement.get()->m_Params.m_Params.size(), std::move(functionArgsTypes), functionStatement.get()->m_ReturnTypeAnnotation.m_ReturnType));
  auto functionBind = std::make_shared<BindingFunction>(BindingFunction(functionStatement.get()->m_Position, functionStatement.get()->m_Identifier.get()->m_Position, functionStatement.get()->m_Params.m_Position, functionType, 0));
  SaveBind(functionStatement.get()->m_Identifier.get()->m_Value, functionBind);

  EnterScope(ScopeType::FUNCTION);

  for (auto &param : functionStatement.get()->m_Params.m_Params)
  {
    if (m_Scopes.back().m_Context.Get(param.m_Identifier.get()->m_Value).has_value())
    {
      DiagnosticReference reference(Errno::OK, 0, m_Scopes.back().m_Context.Get(param.m_Identifier.get()->m_Value).value().get()->m_Position, "first used here");
      m_Diagnostics.push_back(Diagnostic(Errno::NAME_ERROR, param.m_Position, m_ModuleID, DiagnosticSeverity::ERROR, std::format("duplicated param name '{}'", param.m_Identifier->m_Value), reference));
      continue;
    }
    SaveBind(param.m_Identifier.get()->m_Value, std::make_shared<Binding>(Binding(BindType::PARAMETER, param.m_TypeAnnotation.m_ReturnType, m_ModuleID, param.m_Position)));
  }

  auto blockReturnBind = CheckStatementBlock(functionStatement.get()->m_Body);

  // TODO: make deep types comparison
  if (blockReturnBind.has_value())
  {
    if (!type::MatchBaseTypes(blockReturnBind->get()->m_Type.get()->m_Base, functionStatement.get()->m_ReturnTypeAnnotation.m_ReturnType.get()->m_Base))
    {
      DiagnosticReference reference(Errno::OK, 0, functionStatement.get()->m_ReturnTypeAnnotation.m_Position.value(), std::format("expect '{}' due to here", type::InspectBase(functionStatement.get()->m_ReturnTypeAnnotation.m_ReturnType.get()->m_Base)));
      m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, blockReturnBind->get()->m_Position, m_ModuleID, DiagnosticSeverity::ERROR, std::format("return type mismatch, expect '{}' but got '{}'", type::InspectBase(functionStatement.get()->m_ReturnTypeAnnotation.m_ReturnType.get()->m_Base), type::InspectBase(blockReturnBind->get()->m_Type.get()->m_Base)), reference));
    }
  }
  else
  {
    if (!type::MatchBaseTypes(type::Base::VOID, functionStatement.get()->m_ReturnTypeAnnotation.m_ReturnType.get()->m_Base))
    {
      m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, functionStatement.get()->m_ReturnTypeAnnotation.m_Position.value(), m_ModuleID, DiagnosticSeverity::ERROR, "non-void function doesn't have return value"));
    }
  }

  LeaveScope();

  return std::nullopt;
}

std::optional<std::shared_ptr<Binding>> Checker::CheckStatementReturn(std::shared_ptr<StatementReturn> returnStatement)
{
  if (!IsWithinScope(ScopeType::FUNCTION))
  {
    m_Diagnostics.push_back(Diagnostic(Errno::SYNTAX_ERROR, returnStatement.get()->m_Position, m_ModuleID, DiagnosticSeverity::ERROR, "cannot return outside of a function"));
    return std::nullopt;
  }
  auto returnBind = std::make_shared<Binding>(Binding(BindType::RETURN_VALUE, std::make_shared<type::Type>(type::Type(type::Base::VOID)), m_ModuleID, returnStatement.get()->m_Position, true));
  if (returnStatement.get()->m_Value.has_value())
  {
    auto returnValueBind = CheckExpression(returnStatement.get()->m_Value.value());
    if (returnValueBind.has_value())
    {
      returnValueBind->get()->m_IsUsed = true;
      returnBind->m_Type = returnValueBind->get()->m_Type;
    }
    else
    {
      return std::nullopt;
    }
  }
  return returnBind;
}

std::optional<std::shared_ptr<Binding>> Checker::CheckStatementBlock(std::shared_ptr<StatementBlock> blockStatement)
{
  std::vector<std::shared_ptr<Binding>> statementsBinds = {};
  for (size_t i = 0; i < blockStatement.get()->m_Stmts.size(); ++i)
  {
    auto statement = blockStatement->m_Stmts.at(i);
    auto statementBind = CheckStatement(statement);
    if (statementBind.has_value())
    {
      statementsBinds.push_back(statementBind.value());
    }
    if (StatementType::RETURN == statement.get()->m_Type && (i + 1 < blockStatement.get()->m_Stmts.size()))
    {
      Position position = blockStatement.get()->m_Stmts.at(i + 1).get()->m_Position;
      position.m_End = blockStatement.get()->m_Position.m_End - 1;
      m_Diagnostics.push_back(Diagnostic(Errno::DEAD_CODE, position, m_ModuleID, DiagnosticSeverity::WARN, "unreachable code detected"));
      break;
    }
  }

  std::optional<std::shared_ptr<Binding>> returnBind;
  for (auto &bind : statementsBinds)
  {
    if (BindType::RETURN_VALUE == bind.get()->m_BindType)
    {
      returnBind = bind;
    }
    if (!bind.get()->m_IsUsed)
    {
      m_Diagnostics.push_back(Diagnostic(Errno::UNUSED_VALUE, bind.get()->m_Position, m_ModuleID, DiagnosticSeverity::WARN, "expression results to unused value"));
    }
  }
  return returnBind;
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
  if (type::Base::FUNCTION != calleeBind.get()->m_Type.get()->m_Base)
  {
    m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, callExpression.get()->m_Callee.get()->m_Position, m_ModuleID, DiagnosticSeverity::ERROR, "call to non-callable object"));
    return std::nullopt;
  }

  auto callee = std::static_pointer_cast<type::Function>(calleeBind->m_Type);

  if (callee.get()->m_IsVariadicArguments)
  {
    if (callee.get()->m_ReqArgsCount > callExpression.get()->m_Arguments.size())
    {
      m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, callExpression.get()->m_ArgumentsPosition, m_ModuleID, DiagnosticSeverity::ERROR, std::format("missing required arguments, expect {} but got {}", callee.get()->m_ReqArgsCount, callExpression.get()->m_Arguments.size())));
    }
  }
  else if (callee.get()->m_ReqArgsCount != callExpression.get()->m_Arguments.size())
  {
    m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, callExpression.get()->m_ArgumentsPosition, m_ModuleID, DiagnosticSeverity::ERROR, std::format("missing required arguments, expect {} but got {}", callee.get()->m_ReqArgsCount, callExpression.get()->m_Arguments.size())));
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
  auto bind = LookupBind(identifierExpression.get()->m_Value);
  if (bind.has_value())
  {
    bind.value().get()->m_IsUsed = true;
    auto identifierBind = std::make_shared<Binding>(Binding(BindType::IDENTIFIER, bind.value().get()->m_Type, m_ModuleID, identifierExpression.get()->m_Position));
    return identifierBind;
  }
  m_Diagnostics.push_back(Diagnostic(Errno::NAME_ERROR, identifierExpression.get()->m_Position, m_ModuleID, DiagnosticSeverity::ERROR, std::format("undefined name: '{}'", identifierExpression.get()->m_Value)));
  return std::nullopt;
}

std::optional<std::shared_ptr<Binding>> Checker::CheckExpressionString(std::shared_ptr<ExpressionString> stringExpression)
{
  auto stringLiteralBind = std::make_shared<Binding>(Binding(BindType::LITERAL, std::make_shared<type::Type>(type::Type(type::Base::STRING)), m_ModuleID, stringExpression.get()->m_Position));
  return stringLiteralBind;
}

void Checker::EnterScope(ScopeType scopeType)
{
  m_Scopes.push_back(Scope(scopeType));
}

void Checker::LeaveScope()
{
  for (auto &bind : m_Scopes.back().m_Context.Store)
  {
    if (bind.second.get()->m_IsUsed || bind.first.starts_with('_'))
    {
      continue;
    }
    switch (bind.second.get()->m_BindType)
    {
    case BindType::IDENTIFIER:
    case BindType::LITERAL:
    case BindType::RETURN_VALUE:
      /* Values with these Bind types never get stored in the context */
      break;
    case BindType::PARAMETER:
      m_Diagnostics.push_back(Diagnostic(Errno::UNUSED_VALUE, bind.second.get()->m_Position, m_ModuleID, DiagnosticSeverity::WARN, "unused parameter"));
      break;
    case BindType::FUNCTION:
      m_Diagnostics.push_back(Diagnostic(Errno::UNUSED_VALUE, std::static_pointer_cast<BindingFunction>(bind.second).get()->NamePosition, m_ModuleID, DiagnosticSeverity::WARN, std::format("function '{}' never gets called", bind.first)));
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
