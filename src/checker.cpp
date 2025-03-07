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
#include "module.h"
#include "parser.h"
#include "token.h"
#include "type.h"

std::vector<Diagnostic> Checker::Check()
{
  EnterScope(ScopeType::GLOBAL);
  std::vector<std::shared_ptr<Binding>> statementsBinds = {};
  for (auto statement : m_Module.get()->m_AST.value().get()->m_Program)
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
  switch (statement.get()->GetType())
  {
  case StatementType::FUNCTION_SIGNATURE:
    return CheckStatementFunctionSignature(std::static_pointer_cast<StatementFunctionSignature>(statement));
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

std::optional<std::shared_ptr<Binding>> Checker::CheckStatementFunctionSignature(std::shared_ptr<StatementFunctionSignature> functionStatementSignature)
{
  auto bindWithSameName = m_Scopes.back().m_Context.Get(functionStatementSignature.get()->GetName());
  if (bindWithSameName.has_value())
  {
    DiagnosticReference reference(Errno::OK, bindWithSameName.value().get()->m_ModuleID, bindWithSameName.value()->m_Position, "name used here");
    m_Diagnostics.push_back(Diagnostic(Errno::NAME_ERROR, functionStatementSignature.get()->GetNamePosition(), m_Module.get()->m_ID, DiagnosticSeverity::ERROR, std::format("name '{}' already in use", functionStatementSignature.get()->GetName()), reference));
    return std::nullopt;
  }
  std::vector<std::shared_ptr<type::Type>> functionArgsTypes;
  for (auto &param : functionStatementSignature.get()->GetParams())
  {
    if (param.GetAstType().has_value())
    {
      functionArgsTypes.push_back(param.GetAstType().value().GetType());
    }
    else
    {
      functionArgsTypes.push_back(std::make_shared<type::Type>(type::Base::ANY));
    }
  }
  auto returnType = std::make_shared<type::Type>(type::Type(type::Base::VOID));
  if (functionStatementSignature.get()->GetReturnType().has_value())
  {
    returnType = functionStatementSignature.get()->GetReturnType().value().GetType();
  }
  auto functionType = std::make_shared<type::Function>(type::Function(functionStatementSignature.get()->GetParams().size(), std::move(functionArgsTypes), returnType, functionStatementSignature.get()->IsVarArgs()));
  auto functionBind = std::make_shared<BindingFunction>(BindingFunction(functionStatementSignature.get()->GetPosition(), functionStatementSignature.get()->GetNamePosition(), functionStatementSignature.get()->GetParamsPosition(), functionType, m_Module.get()->m_ID, false, functionStatementSignature.get()->IsPub()));
  SaveBind(functionStatementSignature.get()->GetName(), functionBind);
  return std::nullopt;
}

std::optional<std::shared_ptr<Binding>> Checker::CheckStatementFunction(std::shared_ptr<StatementFunction> functionStatement)
{
  CheckStatementFunctionSignature(functionStatement.get()->GetSignature());
  auto returnType = std::make_shared<type::Type>(type::Type(type::Base::VOID));
  if (functionStatement.get()->GetSignature().get()->GetReturnType().has_value())
  {
    returnType = functionStatement.get()->GetSignature().get()->GetReturnType().value().GetType();
  }
  EnterScope(ScopeType::FUNCTION);
  for (auto &param : functionStatement.get()->GetSignature()->GetParams())
  {
    if (m_Scopes.back().m_Context.Get(param.GetName()).has_value())
    {
      DiagnosticReference reference(Errno::OK, m_Scopes.back().m_Context.Get(param.GetName()).value().get()->m_ModuleID, m_Scopes.back().m_Context.Get(param.GetName()).value().get()->m_Position, "first used here");
      m_Diagnostics.push_back(Diagnostic(Errno::NAME_ERROR, param.GetNamePosition(), m_Module.get()->m_ID, DiagnosticSeverity::ERROR, std::format("duplicated param name '{}'", param.GetName()), reference));
      continue;
    }
    auto paramType = std::make_shared<type::Type>(type::Type(type::Base::ANY));
    if (param.GetAstType().has_value())
    {
      paramType = param.GetAstType().value().GetType();
    }
    SaveBind(param.GetName(), std::make_shared<Binding>(Binding(BindType::PARAMETER, paramType, m_Module.get()->m_ID, param.GetPosition())));
  }
  auto blockReturnBind = CheckStatementBlock(functionStatement.get()->GetBody());
  if (blockReturnBind.has_value())
  {
    if (type::Base::VOID == returnType.get()->m_Base)
    {
      m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, blockReturnBind->get()->m_Position, m_Module.get()->m_ID, DiagnosticSeverity::ERROR, "void function does not accept return value"));
    }
    else
    {
      auto returnTypeAnnotation = functionStatement.get()->GetSignature()->GetReturnType().value();
      if (!returnType.get()->IsCompatibleWith(blockReturnBind.value().get()->m_Type))
      {
        DiagnosticReference reference(Errno::OK, m_Module.get()->m_ID, returnTypeAnnotation.GetPosition(), std::format("expect '{}' due to here", returnType.get()->Inspect()));
        m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, blockReturnBind->get()->m_Position, m_Module.get()->m_ID, DiagnosticSeverity::ERROR, std::format("return type mismatch, expect '{}' but got '{}'", returnType.get()->Inspect(), blockReturnBind.value().get()->m_Type.get()->Inspect()), reference));
      }
    }
  }
  else
  {
    if (type::Base::VOID != returnType.get()->m_Base)
    {
      Position returnTypeAnnotationPosition = functionStatement.get()->GetSignature()->GetReturnType().value().GetPosition();
      m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, returnTypeAnnotationPosition, m_Module.get()->m_ID, DiagnosticSeverity::ERROR, "missing return value for non-void function"));
    }
  }
  LeaveScope();
  return std::nullopt;
}

std::optional<std::shared_ptr<Binding>> Checker::CheckStatementReturn(std::shared_ptr<StatementReturn> returnStatement)
{
  if (!IsWithinScope(ScopeType::FUNCTION))
  {
    m_Diagnostics.push_back(Diagnostic(Errno::SYNTAX_ERROR, returnStatement.get()->GetPosition(), m_Module.get()->m_ID, DiagnosticSeverity::ERROR, "cannot return outside of a function"));
    return std::nullopt;
  }
  auto returnBind = std::make_shared<Binding>(Binding(BindType::RETURN_VALUE, std::make_shared<type::Type>(type::Type(type::Base::VOID)), m_Module.get()->m_ID, returnStatement.get()->GetPosition(), true));
  if (returnStatement.get()->GetValue().has_value())
  {
    auto returnValueBind = CheckExpression(returnStatement.get()->GetValue().value());
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
  auto statements = blockStatement.get()->GetStatements();
  for (size_t i = 0; i < statements.size(); ++i)
  {
    auto statement = statements.at(i);
    auto statementBind = CheckStatement(statement);
    if (statementBind.has_value())
    {
      statementsBinds.push_back(statementBind.value());
    }
    if (StatementType::RETURN == statement.get()->GetType() && (i + 1 < statements.size()))
    {
      Position position = statements.at(i + 1).get()->GetPosition();
      position.m_End = blockStatement.get()->GetPosition().m_End - 1;
      m_Diagnostics.push_back(Diagnostic(Errno::DEAD_CODE, position, m_Module.get()->m_ID, DiagnosticSeverity::WARN, "unreachable code detected"));
      break;
    }
  }

  std::optional<std::shared_ptr<Binding>> returnBind;
  for (auto &bind : statementsBinds)
  {
    if (BindType::RETURN_VALUE == bind.get()->m_BindType)
    {
      returnBind = bind;
      continue;
    }
    if (type::Base::VOID == bind.get()->m_Type.get()->m_Base)
    {
      continue;
    }
    m_Diagnostics.push_back(Diagnostic(Errno::UNUSED_VALUE, bind.get()->m_Position, m_Module.get()->m_ID, DiagnosticSeverity::WARN, "expression results to unused value"));
  }
  return returnBind;
}

std::optional<std::shared_ptr<Binding>> Checker::CheckStatementLet(std::shared_ptr<StatementLet> letStatement)
{
  // let name
  auto bindWithSameName = m_Scopes.back().m_Context.Get(letStatement.get()->GetName());
  if (bindWithSameName.has_value())
  {
    DiagnosticReference reference(Errno::OK, bindWithSameName.value().get()->m_ModuleID, bindWithSameName.value()->m_Position, "name used here");
    m_Diagnostics.push_back(Diagnostic(Errno::NAME_ERROR, letStatement.get()->GetNamePosition(), m_Module.get()->m_ID, DiagnosticSeverity::ERROR, std::format("name '{}' already in use", letStatement.get()->GetName()), reference));
    return std::nullopt;
  }
  // let type
  auto letType = std::make_shared<type::Type>(type::Type(type::Base::ANY));
  if (letStatement.get()->GetAstType().has_value())
  {
    letType = letStatement.get()->GetAstType().value().GetType();
  }
  // let initializer
  if (letStatement.get()->GetInitializer().has_value())
  {
    auto initializerBindOpt = CheckExpression(letStatement.get()->GetInitializer().value());
    if (!initializerBindOpt.has_value())
    {
      goto defer;
    }
    auto initializerBind = initializerBindOpt.value();
    if (letType.get()->IsCompatibleWith(initializerBind.get()->m_Type))
    {
      letType = initializerBind.get()->m_Type;
    }
    else
    {
      Position letTypeAnnotationPosition = letStatement.get()->GetAstType().value().GetPosition();
      DiagnosticReference reference(Errno::OK, m_Module.get()->m_ID, letTypeAnnotationPosition, std::format("expect '{}' due to here", letType.get()->Inspect()));
      m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, initializerBind.get()->m_Position, initializerBind.get()->m_ModuleID, DiagnosticSeverity::ERROR, std::format("expect value of type '{}' but got '{}'", letType.get()->Inspect(), initializerBind.get()->m_Type.get()->Inspect()), reference));
    }
  }
defer:
  SaveBind(letStatement.get()->GetName(), std::make_shared<Binding>(Binding(BindType::VARIABLE, letType, m_Module.get()->m_ID, letStatement.get()->GetNamePosition(), false, letStatement.get()->IsPub())));
  return std::nullopt;
}

std::optional<std::shared_ptr<Binding>> Checker::CheckStatementImport(std::shared_ptr<StatementImport> importStatement)
{
  auto bindWithSameName = m_Scopes.back().m_Context.Get(importStatement.get()->GetName());
  if (bindWithSameName.has_value())
  {
    DiagnosticReference reference(Errno::OK, bindWithSameName.value().get()->m_ModuleID, bindWithSameName.value()->m_Position, "name used here");
    m_Diagnostics.push_back(Diagnostic(Errno::NAME_ERROR, importStatement.get()->GetNamePosition(), m_Module.get()->m_ID, DiagnosticSeverity::ERROR, std::format("name '{}' already in use", importStatement.get()->GetName()), reference));
    return std::nullopt;
  }
  auto loadRes = m_ModManager.Load(importStatement.get()->GetPath());
  if (loadRes.is_err())
  {
    m_Diagnostics.push_back(Diagnostic(Errno::NAME_ERROR, importStatement.get()->GetPathPosition(), m_Module.get()->m_ID, DiagnosticSeverity::ERROR, std::format("failed to import module '{}', '{}'", importStatement.get()->GetName(), loadRes.unwrap_err().Message)));
    return std::nullopt;
  }
  auto module = loadRes.unwrap();
  if (ModuleStatus::INVALID == module.get()->m_Status)
  {
    return std::nullopt;
  }
  if (ModuleStatus::IDLE == module.get()->m_Status)
  {
    Parser parser(module, m_ModManager);
    auto parseError = parser.Parse();
    if (parseError.has_value())
    {
      module.get()->m_Status = ModuleStatus::INVALID;
      m_Diagnostics.push_back(parseError.value());
      return std::nullopt;
    }
    auto exports = std::make_shared<ModuleContext>(ModuleContext());
    Checker checker(module, m_ModManager);
    auto diagnostics = checker.Check();
    m_Diagnostics.insert(m_Diagnostics.end(), diagnostics.begin(), diagnostics.end());
    module.get()->m_Status = ModuleStatus::LOADED;
  }
  auto objectType = std::make_shared<type::Object>(type::Object());
  for (auto &bind : module.get()->m_Exports.value().get()->Store)
  {
    objectType.get()->m_Entries[bind.first] = bind.second.get()->m_Type;
  }
  auto moduleBind = std::make_shared<BindingModule>(BindingModule(importStatement.get()->GetPosition(), importStatement.get()->GetNamePosition(), m_Module.get()->m_ID, module.get()->m_Exports.value(), objectType));
  SaveBind(importStatement.get()->GetName(), moduleBind);
  return std::nullopt;
}

std::optional<std::shared_ptr<Binding>> Checker::CheckExpression(std::shared_ptr<Expression> expression)
{
  switch (expression.get()->GetType())
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
  auto calleeBindOpt = CheckExpression(callExpression.get()->GetCallee());
  if (!calleeBindOpt.has_value())
  {
    return std::nullopt;
  }
  auto calleeBind = calleeBindOpt.value();
  if (type::Base::FUNCTION != calleeBind.get()->m_Type.get()->m_Base)
  {
    m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, callExpression.get()->GetCalleePosition(), m_Module.get()->m_ID, DiagnosticSeverity::ERROR, "call to non-callable object"));
    return std::nullopt;
  }
  auto calleeFnType = std::static_pointer_cast<type::Function>(calleeBind->m_Type);
  // arguments
  auto callExpressionArgs = callExpression.get()->GetArguments();
  auto callExpressionArgsPosition = callExpression.get()->GetArgumentsPosition();
  if (calleeFnType.get()->m_IsVariadicArguments)
  {
    if (calleeFnType.get()->m_ReqArgsCount > callExpressionArgs.size())
    {
      m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, callExpressionArgsPosition, m_Module.get()->m_ID, DiagnosticSeverity::ERROR, std::format("missing required arguments, expect {} but got {}", calleeFnType.get()->m_ReqArgsCount, callExpressionArgs.size())));
      return std::nullopt;
    }
  }
  else if (calleeFnType.get()->m_ReqArgsCount != callExpressionArgs.size())
  {
    m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, callExpressionArgsPosition, m_Module.get()->m_ID, DiagnosticSeverity::ERROR, std::format("arguments count mismatch, expect {} but got {}", calleeFnType.get()->m_ReqArgsCount, callExpressionArgs.size())));
    return std::nullopt;
  }
  for (size_t i = 0; i < callExpressionArgs.size(); ++i)
  {
    auto argumentBind = CheckExpression(callExpressionArgs.at(i));
    if (!argumentBind.has_value())
    {
      continue;
    }
    argumentBind.value().get()->m_IsUsed = true;
    if (i >= calleeFnType.get()->m_Arguments.size())
    {
      continue;
    }
    if (calleeFnType.get()->m_Arguments.at(i).get()->IsCompatibleWith(argumentBind.value().get()->m_Type))
    {
      continue;
    }
    m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, argumentBind.value().get()->m_Position, m_Module.get()->m_ID, DiagnosticSeverity::ERROR, std::format("expect argument of type '{}' but got '{}'", calleeFnType.get()->m_Arguments.at(i).get()->Inspect(), argumentBind.value().get()->m_Type.get()->Inspect())));
  }
  return std::make_shared<Binding>(Binding(BindType::EXPRESSION, calleeFnType.get()->m_ReturnType, m_Module.get()->m_ID, callExpression.get()->GetPosition()));
}

std::optional<std::shared_ptr<Binding>> Checker::CheckExpressionIdentifier(std::shared_ptr<ExpressionIdentifier> identifierExpression)
{
  auto bind = LookupBind(identifierExpression.get()->GetValue());
  if (bind.has_value())
  {
    bind.value().get()->m_IsUsed = true;
    auto identifierBind = std::make_shared<Binding>(Binding(BindType::EXPRESSION, bind.value().get()->m_Type, m_Module.get()->m_ID, identifierExpression.get()->GetPosition()));
    return identifierBind;
  }
  m_Diagnostics.push_back(Diagnostic(Errno::NAME_ERROR, identifierExpression.get()->GetPosition(), m_Module.get()->m_ID, DiagnosticSeverity::ERROR, std::format("undefined name: '{}'", identifierExpression.get()->GetValue())));
  return std::nullopt;
}

std::optional<std::shared_ptr<Binding>> Checker::CheckExpressionAssign(std::shared_ptr<ExpressionAssign> assignExpression)
{
  // assignee
  auto assigneeBindOpt = CheckExpressionIdentifier(assignExpression.get()->GetAssignee());
  if (!assigneeBindOpt.has_value())
  {
    return std::nullopt;
  }
  auto assigneeBind = assigneeBindOpt.value();
  // value
  auto valueBindOpt = CheckExpression(assignExpression.get()->GetValue());
  if (!valueBindOpt.has_value())
  {
    return std::nullopt;
  }
  auto valueBind = valueBindOpt.value();
  // match types
  if (!assigneeBind.get()->m_Type.get()->IsCompatibleWith(valueBind.get()->m_Type))
  {
    m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, valueBind.get()->m_Position, valueBind.get()->m_ModuleID, DiagnosticSeverity::ERROR, std::format("expect value of type '{}' but got '{}'", assigneeBind.get()->m_Type.get()->Inspect(), valueBind.get()->m_Type.get()->Inspect())));
    return std::nullopt;
  }
  // narrow types
  valueBind.get()->m_IsUsed = true;
  assigneeBind.get()->m_Type = valueBind.get()->m_Type;
  return assigneeBind;
}

std::optional<std::shared_ptr<Binding>> Checker::CheckExpressionFieldAccess(std::shared_ptr<ExpressionFieldAccess> fieldAccessExpression)
{
  auto valueBindOpt = CheckExpression(fieldAccessExpression.get()->GetValue());
  if (!valueBindOpt.has_value())
  {
    return std::nullopt;
  }
  auto valueBind = valueBindOpt.value();
  if (type::Base::OBJECT != valueBind.get()->m_Type.get()->m_Base)
  {
    m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, fieldAccessExpression.get()->GetPosition(), m_Module.get()->m_ID, DiagnosticSeverity::ERROR, "left side of field access must be an object"));
    return std::nullopt;
  }
  auto bindObjectType = std::static_pointer_cast<type::Object>(valueBind.get()->m_Type);
  if (bindObjectType.get()->m_Entries.find(fieldAccessExpression.get()->GetFieldName().get()->GetValue()) == bindObjectType.get()->m_Entries.end())
  {
    m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, fieldAccessExpression.get()->GetFieldName().get()->GetPosition(), m_Module.get()->m_ID, DiagnosticSeverity::ERROR, std::format("object '{}' has no field '{}'", bindObjectType.get()->Inspect(), fieldAccessExpression.get()->GetFieldName().get()->GetValue())));
    return std::nullopt;
  }

  auto bind = std::make_shared<Binding>(Binding(BindType::EXPRESSION, bindObjectType.get()->m_Entries.at(fieldAccessExpression.get()->GetFieldName().get()->GetValue()), m_Module.get()->m_ID, fieldAccessExpression.get()->GetFieldName().get()->GetPosition()));
  return bind;
}

std::optional<std::shared_ptr<Binding>> Checker::CheckExpressionString(std::shared_ptr<ExpressionString> stringExpression)
{
  auto stringLiteralBind = std::make_shared<Binding>(Binding(BindType::EXPRESSION, std::make_shared<type::Type>(type::Type(type::Base::STRING)), m_Module.get()->m_ID, stringExpression.get()->GetPosition()));
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
    if (bind.second.get()->m_IsUsed || bind.first.starts_with('_') || bind.second.get()->m_IsPub)
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
    m_Module.get()->m_Exports = std::make_shared<ModuleContext>(ModuleContext());
    for (auto &pair : m_Scopes.back().m_Context.Store)
    {
      if (pair.second.get()->m_IsPub)
      {
        m_Module.get()->m_Exports.value().get()->Store[pair.first] = pair.second;
      }
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
