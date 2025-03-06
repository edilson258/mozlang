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
#include "lexer.h"
#include "parser.h"
#include "token.h"
#include "type.h"

std::vector<Diagnostic> Checker::Check()
{
  EnterScope(ScopeType::GLOBAL);

  auto printlnArgs = {std::make_shared<type::Type>(type::Base::F_STRING)};
  auto printlnType = std::make_shared<type::Function>(type::Function(0, std::move(printlnArgs), std::make_shared<type::Type>(type::Base::VOID), true));
  // TODO: module ID
  auto printlnBind = std::make_shared<BindingFunction>(Position(), Position(), Position(), printlnType, 0, true);
  SaveBind("println", printlnBind);

  std::vector<std::shared_ptr<Binding>> statementsBinds = {};
  for (auto statement : m_Ast.m_Program)
  {
    auto statementBind = CheckStatement(statement);
    if (statementBind.has_value())
    {
      statementsBinds.push_back(statementBind.value());
    }
  }
  for (auto &bind : statementsBinds)
  {
    if (type::Base::VOID == bind.get()->m_Type.get()->m_Base)
    {
      continue;
    }
    m_Diagnostics.push_back(Diagnostic(Errno::UNUSED_VALUE, bind.get()->m_Position, bind.get()->m_ModuleID, DiagnosticSeverity::WARN, "expression results to unused value"));
  }

  LeaveScope();

  return std::move(m_Diagnostics);
}

std::optional<std::shared_ptr<Binding>> Checker::CheckStatement(std::shared_ptr<Statement> statement)
{
  switch (statement.get()->m_Type)
  {
  case StatementType::IMPORT:
    return CheckStatementImport(std::static_pointer_cast<StatementImport>(statement));
  case StatementType::LET:
    return CheckStatementLet(std::static_pointer_cast<StatementLet>(statement));
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
    DiagnosticReference reference(Errno::OK, bindWithSameName.value().get()->m_ModuleID, bindWithSameName.value()->m_Position, "name used here");
    m_Diagnostics.push_back(Diagnostic(Errno::NAME_ERROR, functionStatement.get()->m_Identifier.get()->m_Position, m_ModuleID, DiagnosticSeverity::ERROR, std::format("name '{}' already in use", functionStatement.get()->m_Identifier.get()->m_Value), reference));
    return std::nullopt;
  }
  std::vector<std::shared_ptr<type::Type>> functionArgsTypes;
  for (auto &param : functionStatement.get()->m_Params.m_Params)
  {
    if (param.m_AstTypeAnnotationOpt.has_value())
    {
      functionArgsTypes.push_back(param.m_AstTypeAnnotationOpt.value().m_Type);
    }
    else
    {
      functionArgsTypes.push_back(std::make_shared<type::Type>(type::Base::ANY));
    }
  }
  auto returnType = std::make_shared<type::Type>(type::Type(type::Base::VOID));
  if (functionStatement.get()->m_ReturnTypeAnnotationOpt.has_value())
  {
    returnType = functionStatement.get()->m_ReturnTypeAnnotationOpt.value().m_Type;
  }
  auto functionType = std::make_shared<type::Function>(type::Function(functionStatement.get()->m_Params.m_Params.size(), std::move(functionArgsTypes), returnType));
  auto functionBind = std::make_shared<BindingFunction>(BindingFunction(functionStatement.get()->m_Position, functionStatement.get()->m_Identifier.get()->m_Position, functionStatement.get()->m_Params.m_Position, functionType, m_ModuleID));
  SaveBind(functionStatement.get()->m_Identifier.get()->m_Value, functionBind);
  EnterScope(ScopeType::FUNCTION);
  for (auto &param : functionStatement.get()->m_Params.m_Params)
  {
    if (m_Scopes.back().m_Context.Get(param.m_Identifier.get()->m_Value).has_value())
    {
      DiagnosticReference reference(Errno::OK, m_Scopes.back().m_Context.Get(param.m_Identifier.get()->m_Value).value().get()->m_ModuleID, m_Scopes.back().m_Context.Get(param.m_Identifier.get()->m_Value).value().get()->m_Position, "first used here");
      m_Diagnostics.push_back(Diagnostic(Errno::NAME_ERROR, param.m_Position, m_ModuleID, DiagnosticSeverity::ERROR, std::format("duplicated param name '{}'", param.m_Identifier->m_Value), reference));
      continue;
    }
    auto paramType = std::make_shared<type::Type>(type::Type(type::Base::ANY));
    if (param.m_AstTypeAnnotationOpt.has_value())
    {
      paramType = param.m_AstTypeAnnotationOpt.value().m_Type;
    }
    SaveBind(param.m_Identifier.get()->m_Value, std::make_shared<Binding>(Binding(BindType::PARAMETER, paramType, m_ModuleID, param.m_Position)));
  }
  auto blockReturnBind = CheckStatementBlock(functionStatement.get()->m_Body);
  if (blockReturnBind.has_value())
  {
    if (type::Base::VOID == returnType.get()->m_Base)
    {
      m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, blockReturnBind->get()->m_Position, m_ModuleID, DiagnosticSeverity::ERROR, "void function does not accept return value"));
    }
    else
    {
      /* if the return type isn't 'void' we assume that the user explicitly wrote the return type like 'fun foo(): i32 {...}'
       *                                                                                                           ^^^
       */
      auto returnTypeAnnotation = functionStatement.get()->m_ReturnTypeAnnotationOpt.value();
      if (!type::MatchBaseTypes(returnType.get()->m_Base, blockReturnBind->get()->m_Type.get()->m_Base))
      {
        DiagnosticReference reference(Errno::OK, m_ModuleID, functionStatement.get()->m_ReturnTypeAnnotationOpt.value().m_Token.m_Position, std::format("expect '{}' due to here", type::InspectBase(functionStatement.get()->m_ReturnTypeAnnotationOpt.value().m_Type.get()->m_Base)));
        m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, blockReturnBind->get()->m_Position, m_ModuleID, DiagnosticSeverity::ERROR, std::format("return type mismatch, expect '{}' but got '{}'", type::InspectBase(returnType.get()->m_Base), type::InspectBase(blockReturnBind->get()->m_Type.get()->m_Base)), reference));
      }
    }
  }
  else
  {
    if (!type::MatchBaseTypes(type::Base::VOID, returnType.get()->m_Base))
    {
      Position returnTypeAnnotationPosition = functionStatement.get()->m_ReturnTypeAnnotationOpt.value().m_Token.m_Position;
      m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, returnTypeAnnotationPosition, m_ModuleID, DiagnosticSeverity::ERROR, "non-void function doesn't have return value"));
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
    if (type::Base::VOID == bind.get()->m_Type.get()->m_Base)
    {
      continue;
    }
    m_Diagnostics.push_back(Diagnostic(Errno::UNUSED_VALUE, bind.get()->m_Position, m_ModuleID, DiagnosticSeverity::WARN, "expression results to unused value"));
  }
  return returnBind;
}

std::optional<std::shared_ptr<Binding>> Checker::CheckStatementLet(std::shared_ptr<StatementLet> letStatement)
{
  // let name
  auto bindWithSameName = m_Scopes.back().m_Context.Get(letStatement.get()->m_Identifier.get()->m_Value);
  if (bindWithSameName.has_value())
  {
    DiagnosticReference reference(Errno::OK, bindWithSameName.value().get()->m_ModuleID, bindWithSameName.value()->m_Position, "name used here");
    m_Diagnostics.push_back(Diagnostic(Errno::NAME_ERROR, letStatement.get()->m_Identifier.get()->m_Position, m_ModuleID, DiagnosticSeverity::ERROR, std::format("name '{}' already in use", letStatement.get()->m_Identifier.get()->m_Value), reference));
    return std::nullopt;
  }
  // let type
  auto letType = std::make_shared<type::Type>(type::Type(type::Base::ANY));
  if (letStatement.get()->m_AstTypeAnnotation.has_value())
  {
    letType = letStatement.get()->m_AstTypeAnnotation.value().m_Type;
  }
  // let initializer
  if (letStatement.get()->m_Initializer.has_value())
  {
    auto initializerBindOpt = CheckExpression(letStatement.get()->m_Initializer.value());
    if (!initializerBindOpt.has_value())
    {
      goto defer;
    }
    auto initializerBind = initializerBindOpt.value();
    if (type::MatchBaseTypes(letType.get()->m_Base, initializerBind.get()->m_Type.get()->m_Base))
    {
      letType = initializerBind.get()->m_Type;
    }
    else
    {
      Position letTypeAnnotationPosition = letStatement.get()->m_AstTypeAnnotation.value().m_Token.m_Position;
      DiagnosticReference reference(Errno::OK, m_ModuleID, letTypeAnnotationPosition, std::format("expect '{}' due to here", type::InspectBase(letType.get()->m_Base)));
      m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, initializerBind.get()->m_Position, initializerBind.get()->m_ModuleID, DiagnosticSeverity::ERROR, std::format("expect value of type '{}' but got '{}'", type::InspectBase(letType.get()->m_Base), type::InspectBase(initializerBind.get()->m_Type.get()->m_Base)), reference));
    }
  }
defer:
  SaveBind(letStatement.get()->m_Identifier.get()->m_Value, std::make_shared<Binding>(Binding(BindType::VARIABLE, letType, m_ModuleID, letStatement.get()->m_Identifier.get()->m_Position)));
  return std::nullopt;
}

std::optional<std::shared_ptr<Binding>> Checker::CheckStatementImport(std::shared_ptr<StatementImport> importStatement)
{
  auto bindWithSameName = m_Scopes.back().m_Context.Get(importStatement.get()->m_Alias.get()->m_Value);
  if (bindWithSameName.has_value())
  {
    DiagnosticReference reference(Errno::OK, bindWithSameName.value().get()->m_ModuleID, bindWithSameName.value()->m_Position, "name used here");
    m_Diagnostics.push_back(Diagnostic(Errno::NAME_ERROR, importStatement.get()->m_Alias.get()->m_Position, m_ModuleID, DiagnosticSeverity::ERROR, std::format("name '{}' already in use", importStatement.get()->m_Alias.get()->m_Value), reference));
    return std::nullopt;
  }
  auto loadRes = m_ModManager.Load(importStatement.get()->m_Path);
  if (loadRes.is_err())
  {
    m_Diagnostics.push_back(Diagnostic(Errno::NAME_ERROR, importStatement.get()->m_Position, m_ModuleID, DiagnosticSeverity::ERROR, std::format("failed to import module '{}', '{}'", importStatement.get()->m_Alias.get()->m_Value, loadRes.unwrap_err().Message)));
    return std::nullopt;
  }
  ModuleID moduleID = loadRes.unwrap();
  Lexer lexer(moduleID, m_ModManager);
  Parser parser(moduleID, lexer);
  auto parseRes = parser.Parse();
  if (parseRes.is_err())
  {
    m_Diagnostics.push_back(parseRes.unwrap_err());
    return std::nullopt;
  }
  auto ast = parseRes.unwrap();
  auto outContext = std::make_shared<Context>(Context());
  Checker checker(ast, m_ModManager, outContext);
  auto diagnostics = checker.Check();
  m_Diagnostics.insert(m_Diagnostics.end(), diagnostics.begin(), diagnostics.end());
  auto objectType = std::make_shared<type::Object>(type::Object());
  for (auto &bind : outContext.get()->Store)
  {
    objectType.get()->m_Entries[bind.first] = bind.second.get()->m_Type;
  }
  auto moduleBind = std::make_shared<BindingModule>(BindingModule(importStatement.get()->m_Position, importStatement.get()->m_Alias.get()->m_Position, m_ModuleID, outContext, objectType));
  SaveBind(importStatement.get()->m_Alias.get()->m_Value, moduleBind);
  return std::nullopt;
}

std::optional<std::shared_ptr<Binding>> Checker::CheckExpression(std::shared_ptr<Expression> expression)
{
  switch (expression.get()->m_Type)
  {
  case ExpressionType::FIELD_ACCESS:
    return CheckExpressionFieldAccess(std::static_pointer_cast<ExpressionFieldAccess>(expression));
  case ExpressionType::ASSIGN:
    return CheckExpressionAssign(std::static_pointer_cast<ExpressionAssign>(expression));
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
  // callee
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
  auto calleeFnType = std::static_pointer_cast<type::Function>(calleeBind->m_Type);
  // arguments
  if (calleeFnType.get()->m_IsVariadicArguments)
  {
    if (calleeFnType.get()->m_ReqArgsCount > callExpression.get()->m_Arguments.size())
    {
      m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, callExpression.get()->m_ArgumentsPosition, m_ModuleID, DiagnosticSeverity::ERROR, std::format("missing required arguments, expect {} but got {}", calleeFnType.get()->m_ReqArgsCount, callExpression.get()->m_Arguments.size())));
      return std::nullopt;
    }
  }
  else if (calleeFnType.get()->m_ReqArgsCount != callExpression.get()->m_Arguments.size())
  {
    m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, callExpression.get()->m_ArgumentsPosition, m_ModuleID, DiagnosticSeverity::ERROR, std::format("arguments count mismatch, expect {} but got {}", calleeFnType.get()->m_ReqArgsCount, callExpression.get()->m_Arguments.size())));
    return std::nullopt;
  }
  for (size_t i = 0; i < callExpression.get()->m_Arguments.size(); ++i)
  {
    auto argumentBind = CheckExpression(callExpression.get()->m_Arguments.at(i));
    if (!argumentBind.has_value())
    {
      continue;
    }
    argumentBind.value().get()->m_IsUsed = true;
    if (i >= calleeFnType.get()->m_Arguments.size())
    {
      continue;
    }
    if (type::MatchBaseTypes(calleeFnType.get()->m_Arguments.at(i).get()->m_Base, argumentBind.value().get()->m_Type.get()->m_Base))
    {
      continue;
    }
    m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, argumentBind.value().get()->m_Position, m_ModuleID, DiagnosticSeverity::ERROR, std::format("expect argument of type '{}' but got '{}'", type::InspectBase(calleeFnType.get()->m_Arguments.at(i).get()->m_Base), type::InspectBase(argumentBind.value().get()->m_Type.get()->m_Base))));
  }
  return std::make_shared<Binding>(Binding(BindType::EXPRESSION, calleeFnType.get()->m_ReturnType, m_ModuleID, callExpression.get()->m_Position));
}

std::optional<std::shared_ptr<Binding>> Checker::CheckExpressionIdentifier(std::shared_ptr<ExpressionIdentifier> identifierExpression)
{
  auto bind = LookupBind(identifierExpression.get()->m_Value);
  if (bind.has_value())
  {
    bind.value().get()->m_IsUsed = true;
    auto identifierBind = std::make_shared<Binding>(Binding(BindType::EXPRESSION, bind.value().get()->m_Type, m_ModuleID, identifierExpression.get()->m_Position));
    return identifierBind;
  }
  m_Diagnostics.push_back(Diagnostic(Errno::NAME_ERROR, identifierExpression.get()->m_Position, m_ModuleID, DiagnosticSeverity::ERROR, std::format("undefined name: '{}'", identifierExpression.get()->m_Value)));
  return std::nullopt;
}

std::optional<std::shared_ptr<Binding>> Checker::CheckExpressionAssign(std::shared_ptr<ExpressionAssign> assignExpression)
{
  // assignee
  auto assigneeBindOpt = CheckExpressionIdentifier(assignExpression.get()->m_Assignee);
  if (!assigneeBindOpt.has_value())
  {
    return std::nullopt;
  }
  auto assigneeBind = assigneeBindOpt.value();
  // value
  auto valueBindOpt = CheckExpression(assignExpression.get()->m_Value);
  if (!valueBindOpt.has_value())
  {
    return std::nullopt;
  }
  auto valueBind = valueBindOpt.value();
  // match types
  if (!type::MatchBaseTypes(assigneeBind.get()->m_Type.get()->m_Base, valueBind.get()->m_Type.get()->m_Base))
  {
    m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, valueBind.get()->m_Position, valueBind.get()->m_ModuleID, DiagnosticSeverity::ERROR, std::format("expect value of type '{}' but got '{}'", type::InspectBase(assigneeBind.get()->m_Type.get()->m_Base), type::InspectBase(valueBind.get()->m_Type.get()->m_Base))));
    return std::nullopt;
  }
  // narrow types
  valueBind.get()->m_IsUsed = true;
  assigneeBind.get()->m_Type = valueBind.get()->m_Type;
  return assigneeBind;
}

std::optional<std::shared_ptr<Binding>> Checker::CheckExpressionFieldAccess(std::shared_ptr<ExpressionFieldAccess> fieldAccessExpression)
{
  auto valueBindOpt = CheckExpression(fieldAccessExpression.get()->m_Value);
  if (!valueBindOpt.has_value())
  {
    return std::nullopt;
  }
  auto valueBind = valueBindOpt.value();
  if (type::Base::OBJECT != valueBind.get()->m_Type.get()->m_Base)
  {
    m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, fieldAccessExpression.get()->m_Position, m_ModuleID, DiagnosticSeverity::ERROR, "left side of field access must be an object"));
    return std::nullopt;
  }
  auto bindObjectType = std::static_pointer_cast<type::Object>(valueBind.get()->m_Type);
  if (bindObjectType.get()->m_Entries.find(fieldAccessExpression.get()->m_FieldName.get()->m_Value) == bindObjectType.get()->m_Entries.end())
  {
    m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, fieldAccessExpression.get()->m_FieldName.get()->m_Position, m_ModuleID, DiagnosticSeverity::ERROR, std::format("object has no field '{}'", fieldAccessExpression.get()->m_FieldName.get()->m_Value)));
    return std::nullopt;
  }

  auto bind = std::make_shared<Binding>(Binding(BindType::EXPRESSION, bindObjectType.get()->m_Entries.at(fieldAccessExpression.get()->m_FieldName.get()->m_Value), m_ModuleID, fieldAccessExpression.get()->m_Position));
  return bind;
}

std::optional<std::shared_ptr<Binding>> Checker::CheckExpressionString(std::shared_ptr<ExpressionString> stringExpression)
{
  auto stringLiteralBind = std::make_shared<Binding>(Binding(BindType::EXPRESSION, std::make_shared<type::Type>(type::Type(type::Base::STRING)), m_ModuleID, stringExpression.get()->m_Position));
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
    case BindType::MODULE:
      break;
    case BindType::EXPRESSION:
    case BindType::RETURN_VALUE:
      /* Values with these Bind types never get stored in the context */
      break;
    case BindType::VARIABLE:
      m_Diagnostics.push_back(Diagnostic(Errno::UNUSED_VALUE, bind.second.get()->m_Position, bind.second.get()->m_ModuleID, DiagnosticSeverity::WARN, "unused variable"));
      break;
    case BindType::PARAMETER:
      m_Diagnostics.push_back(Diagnostic(Errno::UNUSED_VALUE, bind.second.get()->m_Position, bind.second.get()->m_ModuleID, DiagnosticSeverity::WARN, "unused parameter"));
      break;
    case BindType::FUNCTION:
      m_Diagnostics.push_back(Diagnostic(Errno::UNUSED_VALUE, std::static_pointer_cast<BindingFunction>(bind.second).get()->NamePosition, bind.second.get()->m_ModuleID, DiagnosticSeverity::WARN, std::format("function '{}' never gets called", bind.first)));
      break;
    }
  }

  if (ScopeType::GLOBAL == m_Scopes.back().m_Type)
  {
    for (auto &pair : m_Scopes.back().m_Context.Store)
    {
      m_OutContext.get()->Store[pair.first] = pair.second;
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
