#include <cassert>
#include <cstddef>
#include <format>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
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
    auto statementBind = CheckStmt(statement);
    if (statementBind.has_value())
    {
      statementsBinds.push_back(statementBind.value());
    }
  }
  for (auto &bind : statementsBinds)
  {
    if (type::Base::VOID == bind.get()->m_Type.get()->m_Base || bind.get()->m_IsUsed)
    {
      continue;
    }
    m_Diagnostics.push_back(Diagnostic(Errno::UNUSED_VALUE, bind.get()->m_Position, bind.get()->m_ModuleID, DiagnosticSeverity::WARN, "expression results to unused value"));
  }
  LeaveScope();
  return std::move(m_Diagnostics);
}

std::optional<std::shared_ptr<Binding>> Checker::CheckStmt(std::shared_ptr<Stmt> stmt)
{
  switch (stmt.get()->GetType())
  {
  case StmtT::FunSign:
    return CheckStmtFunSign(std::static_pointer_cast<FunSignStmt>(stmt));
  case StmtT::Import:
    return CheckStmtImport(std::static_pointer_cast<ImportStmt>(stmt));
  case StmtT::Let:
    return CheckStmtLet(std::static_pointer_cast<LetStmt>(stmt));
  case StmtT::Block:
    return CheckStmtBlock(std::static_pointer_cast<BlockStmt>(stmt));
  case StmtT::Ret:
    return CheckStmtRet(std::static_pointer_cast<RetStmt>(stmt));
  case StmtT::Fun:
    return CheckStmtFun(std::static_pointer_cast<FunStmt>(stmt));
  case StmtT::Expr:
    return CheckExpr(std::static_pointer_cast<Expr>(stmt));
  }
  return std::nullopt;
}

std::optional<std::shared_ptr<Binding>> Checker::CheckStmtFunSign(std::shared_ptr<FunSignStmt> funStmtSign)
{
  auto bindWithSameName = m_Scopes.back().m_Context.Get(funStmtSign.get()->GetName());
  if (bindWithSameName.has_value())
  {
    DiagnosticReference reference(Errno::OK, bindWithSameName.value().get()->m_ModuleID, bindWithSameName.value()->m_Position, "name used here");
    m_Diagnostics.push_back(Diagnostic(Errno::NAME_ERROR, funStmtSign.get()->GetNamePosition(), m_Module.get()->m_ID, DiagnosticSeverity::ERROR, std::format("name '{}' already in use", funStmtSign.get()->GetName()), reference));
    return std::nullopt;
  }
  std::vector<std::shared_ptr<type::Type>> functionArgsTypes;
  for (auto &param : funStmtSign.get()->GetParams())
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
  if (funStmtSign.get()->GetReturnType().has_value())
  {
    returnType = funStmtSign.get()->GetReturnType().value().GetType();
  }
  auto functionType = std::make_shared<type::Function>(type::Function(funStmtSign.get()->GetParams().size(), std::move(functionArgsTypes), returnType, funStmtSign.get()->IsVarArgs()));
  auto functionBind = std::make_shared<BindingFunction>(BindingFunction(funStmtSign.get()->GetPosition(), funStmtSign.get()->GetNamePosition(), funStmtSign.get()->GetParamsPosition(), functionType, m_Module.get()->m_ID, false, funStmtSign.get()->IsPub()));
  SaveBind(funStmtSign.get()->GetName(), functionBind);
  return std::nullopt;
}

std::optional<std::shared_ptr<Binding>> Checker::CheckStmtFun(std::shared_ptr<FunStmt> funStmt)
{
  CheckStmtFunSign(funStmt.get()->GetSignature());
  auto returnType = std::make_shared<type::Type>(type::Type(type::Base::VOID));
  if (funStmt.get()->GetSignature().get()->GetReturnType().has_value())
  {
    returnType = funStmt.get()->GetSignature().get()->GetReturnType().value().GetType();
  }
  EnterScope(ScopeType::FUNCTION);
  for (auto &param : funStmt.get()->GetSignature()->GetParams())
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
  auto blockReturnBind = CheckStmtBlock(funStmt.get()->GetBody());
  if (blockReturnBind.has_value())
  {
    if (type::Base::VOID == returnType.get()->m_Base)
    {
      m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, blockReturnBind->get()->m_Position, m_Module.get()->m_ID, DiagnosticSeverity::ERROR, "void function does not accept return value"));
    }
    else
    {
      auto returnTypeAnnotation = funStmt.get()->GetSignature()->GetReturnType().value();
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
      Position returnTypeAnnotationPosition = funStmt.get()->GetSignature()->GetReturnType().value().GetPosition();
      m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, returnTypeAnnotationPosition, m_Module.get()->m_ID, DiagnosticSeverity::ERROR, "missing return value for non-void function"));
    }
  }
  LeaveScope();
  return std::nullopt;
}

std::optional<std::shared_ptr<Binding>> Checker::CheckStmtRet(std::shared_ptr<RetStmt> retStmt)
{
  if (!IsWithinScope(ScopeType::FUNCTION))
  {
    m_Diagnostics.push_back(Diagnostic(Errno::SYNTAX_ERROR, retStmt.get()->GetPosition(), m_Module.get()->m_ID, DiagnosticSeverity::ERROR, "cannot return outside of a function"));
    return std::nullopt;
  }
  auto returnBind = std::make_shared<Binding>(Binding(BindType::RETURN_VALUE, std::make_shared<type::Type>(type::Type(type::Base::VOID)), m_Module.get()->m_ID, retStmt.get()->GetPosition(), true));
  if (retStmt.get()->GetValue().has_value())
  {
    auto returnValueBind = CheckExpr(retStmt.get()->GetValue().value());
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

std::optional<std::shared_ptr<Binding>> Checker::CheckStmtBlock(std::shared_ptr<BlockStmt> blockStmt)
{
  std::vector<std::shared_ptr<Binding>> statementsBinds = {};
  auto statements = blockStmt.get()->GetStatements();
  for (size_t i = 0; i < statements.size(); ++i)
  {
    auto statement = statements.at(i);
    auto statementBind = CheckStmt(statement);
    if (statementBind.has_value())
    {
      statementsBinds.push_back(statementBind.value());
    }
    if (StmtT::Ret == statement.get()->GetType() && (i + 1 < statements.size()))
    {
      Position position = statements.at(i + 1).get()->GetPosition();
      position.m_End = blockStmt.get()->GetPosition().m_End - 1;
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

std::optional<std::shared_ptr<Binding>> Checker::CheckStmtLet(std::shared_ptr<LetStmt> letStmt)
{
  // let name
  auto bindWithSameName = m_Scopes.back().m_Context.Get(letStmt.get()->GetName());
  if (bindWithSameName.has_value())
  {
    DiagnosticReference reference(Errno::OK, bindWithSameName.value().get()->m_ModuleID, bindWithSameName.value()->m_Position, "name used here");
    m_Diagnostics.push_back(Diagnostic(Errno::NAME_ERROR, letStmt.get()->GetNamePosition(), m_Module.get()->m_ID, DiagnosticSeverity::ERROR, std::format("name '{}' already in use", letStmt.get()->GetName()), reference));
    return std::nullopt;
  }
  // let type
  auto letType = std::make_shared<type::Type>(type::Type(type::Base::ANY));
  if (letStmt.get()->GetAstType().has_value())
  {
    letType = letStmt.get()->GetAstType().value().GetType();
  }
  // let initializer
  std::optional<std::shared_ptr<Binding>> initBindRefOpt(std::nullopt);
  if (letStmt.get()->GetInitializer().has_value())
  {
    auto initializerBindOpt = CheckExpr(letStmt.get()->GetInitializer().value());
    if (!initializerBindOpt.has_value())
    {
      goto defer;
    }
    auto initializerBind = initializerBindOpt.value();
    if (letType.get()->IsCompatibleWith(initializerBind.get()->m_Type))
    {
      letType = initializerBind.get()->m_Type;
      if (initializerBind.get()->m_Reference.has_value())
      {
        initBindRefOpt.emplace(initializerBind.get()->m_Reference.value());
      }
    }
    else
    {
      Position letTypeAnnotationPosition = letStmt.get()->GetAstType().value().GetPosition();
      DiagnosticReference reference(Errno::OK, m_Module.get()->m_ID, letTypeAnnotationPosition, std::format("expect '{}' due to here", letType.get()->Inspect()));
      m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, initializerBind.get()->m_Position, initializerBind.get()->m_ModuleID, DiagnosticSeverity::ERROR, std::format("expect value of type '{}' but got '{}'", letType.get()->Inspect(), initializerBind.get()->m_Type.get()->Inspect()), reference));
    }
  }
defer:
  SaveBind(letStmt.get()->GetName(), std::make_shared<Binding>(Binding(BindType::VARIABLE, letType, m_Module.get()->m_ID, letStmt.get()->GetNamePosition(), false, letStmt.get()->IsPub(), initBindRefOpt)));
  return std::nullopt;
}

std::optional<std::shared_ptr<Binding>> Checker::CheckStmtImport(std::shared_ptr<ImportStmt> importStmt)
{
  auto bindWithSameName = m_Scopes.back().m_Context.Get(importStmt.get()->GetName());
  if (bindWithSameName.has_value())
  {
    DiagnosticReference reference(Errno::OK, bindWithSameName.value().get()->m_ModuleID, bindWithSameName.value()->m_Position, "name used here");
    m_Diagnostics.push_back(Diagnostic(Errno::NAME_ERROR, importStmt.get()->GetNamePosition(), m_Module.get()->m_ID, DiagnosticSeverity::ERROR, std::format("name '{}' already in use", importStmt.get()->GetName()), reference));
    return std::nullopt;
  }
  auto loadRes = m_ModManager.Load(NormalizeImportPath(importStmt->hasAtNotation(), importStmt->GetPath()));
  if (loadRes.is_err())
  {
    m_Diagnostics.push_back(Diagnostic(Errno::NAME_ERROR, importStmt.get()->GetPathPosition(), m_Module.get()->m_ID, DiagnosticSeverity::ERROR, std::format("failed to import module '{}', '{}'", importStmt.get()->GetName(), loadRes.unwrap_err().Message)));
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
  auto moduleBind = std::make_shared<BindingModule>(BindingModule(importStmt.get()->GetName(), importStmt.get()->GetPosition(), importStmt.get()->GetNamePosition(), m_Module.get()->m_ID, module.get()->m_Exports.value(), objectType));
  SaveBind(importStmt.get()->GetName(), moduleBind);
  return std::nullopt;
}

std::optional<std::shared_ptr<Binding>> Checker::CheckExpr(std::shared_ptr<Expr> expr)
{
  switch (expr.get()->GetType())
  {
  case ExprT::FieldAcc:
    return CheckExprFieldAcc(std::static_pointer_cast<FieldAccExpr>(expr));
  case ExprT::Assign:
    return CheckExprAssign(std::static_pointer_cast<AssignExpr>(expr));
  case ExprT::Call:
    return CheckExprCall(std::static_pointer_cast<CallExpr>(expr));
  case ExprT::String:
    return CheckExprString(std::static_pointer_cast<StringExpr>(expr));
  case ExprT::Ident:
    return CheckExprIdent(std::static_pointer_cast<IdentExpr>(expr));
  }
  return std::nullopt;
}

std::optional<std::shared_ptr<Binding>> Checker::CheckExprCall(std::shared_ptr<CallExpr> callExpr)
{
  // callee
  auto calleeBindOpt = CheckExpr(callExpr.get()->GetCallee());
  if (!calleeBindOpt.has_value())
  {
    return std::nullopt;
  }
  auto calleeBind = calleeBindOpt.value();
  if (type::Base::FUNCTION != calleeBind.get()->m_Type.get()->m_Base)
  {
    m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, callExpr.get()->GetCalleePosition(), m_Module.get()->m_ID, DiagnosticSeverity::ERROR, "call to non-callable object"));
    return std::nullopt;
  }
  auto calleeFnType = std::static_pointer_cast<type::Function>(calleeBind->m_Type);
  // arguments
  auto callExpressionArgs = callExpr.get()->GetArguments();
  auto callExpressionArgsPosition = callExpr.get()->GetArgumentsPosition();
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
    auto argumentBind = CheckExpr(callExpressionArgs.at(i));
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
  return std::make_shared<Binding>(Binding(BindType::EXPRESSION, calleeFnType.get()->m_ReturnType, m_Module.get()->m_ID, callExpr.get()->GetPosition()));
}

std::optional<std::shared_ptr<Binding>> Checker::CheckExprIdent(std::shared_ptr<IdentExpr> identExpr)
{
  auto bind = LookupBind(identExpr.get()->GetValue());
  if (bind.has_value())
  {
    bind.value().get()->m_IsUsed = true;
    auto identifierBind = std::make_shared<Binding>(Binding(BindType::EXPRESSION, bind.value().get()->m_Type, m_Module.get()->m_ID, identExpr.get()->GetPosition(), false, false, bind.value().get()->m_Reference.has_value() ? bind.value().get()->m_Reference.value() : bind.value()));
    return identifierBind;
  }
  m_Diagnostics.push_back(Diagnostic(Errno::NAME_ERROR, identExpr.get()->GetPosition(), m_Module.get()->m_ID, DiagnosticSeverity::ERROR, std::format("undefined name: '{}'", identExpr.get()->GetValue())));
  return std::nullopt;
}

std::optional<std::shared_ptr<Binding>> Checker::CheckExprAssign(std::shared_ptr<AssignExpr> assignExpr)
{
  // assignee
  auto assigneeBindOpt = CheckExprIdent(assignExpr.get()->GetAssignee());
  if (!assigneeBindOpt.has_value())
  {
    return std::nullopt;
  }
  auto assigneeBind = assigneeBindOpt.value();
  // value
  auto valueBindOpt = CheckExpr(assignExpr.get()->GetValue());
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
  // TODO: update type instead of reassigning to allow single modification point
  assigneeBind.get()->m_Type = valueBind.get()->m_Type;
  assigneeBind.get()->m_Reference.value().get()->m_Type = valueBind.get()->m_Type;
  assigneeBind.get()->m_Reference.value().get()->m_Reference.emplace(valueBind.get()->m_Reference.value());
  return std::nullopt;
}

std::optional<std::shared_ptr<Binding>> Checker::CheckExprFieldAcc(std::shared_ptr<FieldAccExpr> fieldAccExpr)
{
  auto valueBindOpt = CheckExpr(fieldAccExpr.get()->GetValue());
  if (!valueBindOpt.has_value())
  {
    return std::nullopt;
  }
  auto valueBind = valueBindOpt.value();
  if (type::Base::OBJECT != valueBind.get()->m_Type.get()->m_Base)
  {
    m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, fieldAccExpr.get()->GetPosition(), m_Module.get()->m_ID, DiagnosticSeverity::ERROR, "left side of field access must be an object"));
    return std::nullopt;
  }
  auto bindObjectType = std::static_pointer_cast<type::Object>(valueBind.get()->m_Type);
  if (bindObjectType.get()->m_Entries.find(fieldAccExpr.get()->GetFieldName().get()->GetValue()) == bindObjectType.get()->m_Entries.end())
  {
    std::string fieldNotFoundErrorMessage;
    switch (valueBind.get()->m_Reference.value().get()->m_BindType)
    {
    case BindType::MODULE:
      fieldNotFoundErrorMessage = std::format("module '{}' has not field '{}'", std::static_pointer_cast<BindingModule>(valueBind.get()->m_Reference.value()).get()->m_Name, fieldAccExpr.get()->GetFieldName().get()->GetValue());
      break;
    default:
      fieldNotFoundErrorMessage = std::format("object '{}' has no field '{}'", bindObjectType.get()->Inspect(), fieldAccExpr.get()->GetFieldName().get()->GetValue());
    }
    m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, fieldAccExpr.get()->GetFieldName().get()->GetPosition(), m_Module.get()->m_ID, DiagnosticSeverity::ERROR, fieldNotFoundErrorMessage));
    return std::nullopt;
  }
  return std::make_shared<Binding>(Binding(BindType::EXPRESSION, bindObjectType.get()->m_Entries.at(fieldAccExpr.get()->GetFieldName().get()->GetValue()), m_Module.get()->m_ID, fieldAccExpr.get()->GetPosition()));
}

std::optional<std::shared_ptr<Binding>> Checker::CheckExprString(std::shared_ptr<StringExpr> stringExpr)
{
  auto stringLiteralBind = std::make_shared<Binding>(Binding(BindType::EXPRESSION, std::make_shared<type::Type>(type::Type(type::Base::STRING)), m_Module.get()->m_ID, stringExpr.get()->GetPosition()));
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
    case BindType::ERROR:
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

// utils
std::string Checker::NormalizeImportPath(bool hasAtNotation, std::vector<std::shared_ptr<IdentExpr>> path)
{
  std::ostringstream oss;
  if (hasAtNotation)
  {
    // TODO: prefix with ZEROLANG_HOME env variable
  }
  for (size_t i = 0; i < path.size(); ++i)
  {
    oss << path.at(i).get()->GetValue();
    if ((i + 1) < path.size())
    {
      oss << "/";
    }
  }
  oss << ".zr";
  return oss.str();
}
