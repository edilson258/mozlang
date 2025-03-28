#include <cassert>
#include <cstddef>
#include <format>
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
  for (auto statement : m_Module->m_AST->m_Program)
  {
    auto bind = CheckStmt(statement);
    if (bind)
    {
      if (type::Base::VOID == bind->m_Type->m_Base || bind->m_IsUsed)
      {
        continue;
      }
      m_Diagnostics.push_back(Diagnostic(Errno::UNUSED_VALUE, bind->m_Pos, bind->m_ModID, DiagnosticSeverity::WARN, "expression results to unused value"));
    }
  }
  LeaveScope();
  return std::move(m_Diagnostics);
}

Bind *Checker::CheckStmt(Stmt *stmt)
{
  switch (stmt->GetType())
  {
  case StmtT::Fun:
    return CheckStmtFun((FunStmt *)stmt);
  case StmtT::Import:
    return CheckStmtImport((ImportStmt *)stmt);
  case StmtT::Let:
    return CheckStmtLet((LetStmt *)stmt);
  case StmtT::Block:
    return CheckStmtBlock((BlockStmt *)stmt);
  case StmtT::Ret:
    return CheckStmtRet((RetStmt *)stmt);
  case StmtT::Expr:
    return CheckExpr((Expr *)stmt);
  }
  return nullptr;
}

Bind *Checker::CheckStmtFun(FunStmt *funStmt)
{
  FunSign sign = funStmt->GetSign();
  if (IsWithinScope(ScopeType::FUNCTION))
  {
    m_Diagnostics.push_back(Diagnostic(Errno::SYNTAX_ERROR, sign.GetNamePosition(), m_Module->m_ID, DiagnosticSeverity::ERROR, "cannot declare a function inside another function"));
    return nullptr;
  }
  // TODO: impl look up locally
  // TODO: bind to pos
  //
  auto bindWithSameName = m_Scopes.back().m_Context.Get(sign.GetName());
  if (bindWithSameName)
  {
    DiagnosticReference reference(Errno::OK, bindWithSameName->m_ModID, bindWithSameName->m_Pos, "name used here");
    m_Diagnostics.push_back(Diagnostic(Errno::NAME_ERROR, sign.GetNamePosition(), m_Module->m_ID, DiagnosticSeverity::ERROR, "name is already in use", reference));
    return nullptr;
  }
  std::vector<type::Type *> funArgsTypes;
  for (auto &param : sign.GetParams())
  {
    funArgsTypes.push_back(param.GetAstType()->GetType());
  }
  auto returnType = sign.GetRetType() ? sign.GetRetType()->GetType() : new type::Type(type::Base::VOID);
  auto functionType = new type::Function(sign.GetParams().size(), std::move(funArgsTypes), returnType, sign.IsVarArgs());
  auto functionBind = new BindFun(sign.GetPosition(), sign.GetNamePosition(), sign.GetParamsPosition(), functionType, m_Module->m_ID, false, sign.IsPub());
  SaveBind(sign.GetName(), functionBind);
  EnterScope(ScopeType::FUNCTION);
  for (auto &param : sign.GetParams())
  {
    if (m_Scopes.back().m_Context.Get(param.GetName()))
    {
      DiagnosticReference reference(Errno::OK, m_Scopes.back().m_Context.Get(param.GetName())->m_ModID, m_Scopes.back().m_Context.Get(param.GetName())->m_Pos, "first used here");
      m_Diagnostics.push_back(Diagnostic(Errno::NAME_ERROR, param.GetNamePosition(), m_Module->m_ID, DiagnosticSeverity::ERROR, "duplicated param name", reference));
      continue;
    }
    auto paramType = param.GetAstType()->GetType();
    SaveBind(param.GetName(), new Bind(BindT::Param, paramType, m_Module->m_ID, param.GetPosition()));
  }
  if (!funStmt->GetBody())
  {
    // is a simple declaration, eg. `pub fun println(...): void;`
    LeaveScope();
    return nullptr;
  }
  auto blockReturnBind = CheckStmtBlock(funStmt->GetBody());
  if (blockReturnBind)
  {
    if (type::Base::VOID == returnType->m_Base)
    {
      m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, blockReturnBind->m_Pos, m_Module->m_ID, DiagnosticSeverity::ERROR, "void function does not accept return value"));
    }
    else
    {
      auto returnTypeAnot = sign.GetRetType();
      if (!returnType->IsCompatibleWith(blockReturnBind->m_Type))
      {
        DiagnosticReference reference(Errno::OK, m_Module->m_ID, returnTypeAnot->GetPosition(), "due to here");
        m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, blockReturnBind->m_Pos, m_Module->m_ID, DiagnosticSeverity::ERROR, "return type mismatch, expect '{}' but got '{}'", reference));
      }
    }
  }
  else
  {
    if (type::Base::VOID != returnType->m_Base)
    {
      Position returnTypeAnnotationPosition = sign.GetRetType()->GetPosition();
      m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, returnTypeAnnotationPosition, m_Module->m_ID, DiagnosticSeverity::ERROR, "missing return value for non-void function"));
    }
  }
  LeaveScope();
  return nullptr;
}

Bind *Checker::CheckStmtRet(RetStmt *retStmt)
{
  if (!IsWithinScope(ScopeType::FUNCTION) && !retStmt->IsImplicity())
  {
    m_Diagnostics.push_back(Diagnostic(Errno::SYNTAX_ERROR, retStmt->GetPosition(), m_Module->m_ID, DiagnosticSeverity::ERROR, "cannot return outside of a function"));
    return nullptr;
  }
  auto returnBind = new Bind(BindT::RetVal, new type::Type(type::Base::VOID), m_Module->m_ID, retStmt->GetPosition(), true);
  if (retStmt->GetValue())
  {
    auto returnValueBind = CheckExpr(retStmt->GetValue());
    if (returnValueBind)
    {
      returnValueBind->m_IsUsed = true;
      returnBind->m_Type = returnValueBind->m_Type;
    }
    else
    {
      return nullptr;
    }
  }
  return returnBind;
}

Bind *Checker::CheckStmtBlock(BlockStmt *blockStmt)
{
  std::vector<Bind *> statementsBinds = {};
  auto statements = blockStmt->GetStatements();
  for (size_t i = 0; i < statements.size(); ++i)
  {
    auto statement = statements.at(i);
    auto statementBind = CheckStmt(statement);
    if (statementBind)
    {
      statementsBinds.push_back(statementBind);
    }
    if (StmtT::Ret == statement->GetType() && (i + 1 < statements.size()))
    {
      Position position = statements.at(i + 1)->GetPosition();
      position.m_End = blockStmt->GetPosition().m_End - 1;
      m_Diagnostics.push_back(Diagnostic(Errno::DEAD_CODE, position, m_Module->m_ID, DiagnosticSeverity::WARN, "unreachable code detected"));
      break;
    }
  }

  Bind *returnBind = nullptr;
  for (auto &bind : statementsBinds)
  {
    if (BindT::RetVal == bind->m_BindT)
    {
      returnBind = bind;
      continue;
    }
    if (type::Base::VOID == bind->m_Type->m_Base)
    {
      continue;
    }
    m_Diagnostics.push_back(Diagnostic(Errno::UNUSED_VALUE, bind->m_Pos, m_Module->m_ID, DiagnosticSeverity::WARN, "expression results to unused value"));
  }
  return returnBind;
}

Bind *Checker::CheckStmtLet(LetStmt *letStmt)
{
  // let name
  auto bindWithSameName = m_Scopes.back().m_Context.Get(letStmt->GetName());
  if (bindWithSameName)
  {
    DiagnosticReference reference(Errno::OK, bindWithSameName->m_ModID, bindWithSameName->m_Pos, "name used here");
    m_Diagnostics.push_back(Diagnostic(Errno::NAME_ERROR, letStmt->GetNamePosition(), m_Module->m_ID, DiagnosticSeverity::ERROR, "name already used", reference));
    return nullptr;
  }
  // let type
  auto letType = new type::Type(type::Base::ANY);
  if (letStmt->GetAstType())
  {
    letType = letStmt->GetAstType()->GetType();
  }
  // let initializer
  Bind *initBindRef = nullptr;
  if (letStmt->GetInit())
  {
    auto init = CheckExpr(letStmt->GetInit());
    if (!init)
    {
      goto defer;
    }
    if (letType->IsCompatibleWith(init->m_Type))
    {
      letType = init->m_Type;
      if (init->m_Ref)
      {
        initBindRef = init->m_Ref;
      }
    }
    else
    {
      Position letTypeAnnotationPosition = letStmt->GetAstType()->GetPosition();
      DiagnosticReference reference(Errno::OK, m_Module->m_ID, letTypeAnnotationPosition, "due to here");
      m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, init->m_Pos, init->m_ModID, DiagnosticSeverity::ERROR, "expect value of type '{}' but got '{}'", reference));
    }
  }
defer:
  SaveBind(letStmt->GetName(), new Bind(BindT::Var, letType, m_Module->m_ID, letStmt->GetNamePosition(), false, letStmt->IsPub(), initBindRef));
  return nullptr;
}

Bind *Checker::CheckStmtImport(ImportStmt *importStmt)
{
  auto bindWithSameName = m_Scopes.back().m_Context.Get(importStmt->GetName());
  if (bindWithSameName)
  {
    DiagnosticReference reference(Errno::OK, bindWithSameName->m_ModID, bindWithSameName->m_Pos, "name used here");
    m_Diagnostics.push_back(Diagnostic(Errno::NAME_ERROR, importStmt->GetNamePosition(), m_Module->m_ID, DiagnosticSeverity::ERROR, "name already used", reference));
    return nullptr;
  }
  auto loadRes = m_ModManager.Load(NormalizeImportPath(importStmt->hasAtNotation(), importStmt->GetPath()));
  if (loadRes.is_err())
  {
    m_Diagnostics.push_back(Diagnostic(Errno::NAME_ERROR, importStmt->GetPathPosition(), m_Module->m_ID, DiagnosticSeverity::ERROR, "failed to import module"));
    return nullptr;
  }
  auto module = loadRes.unwrap();
  if (ModuleStatus::INVALID == module->m_Status)
  {
    return nullptr;
  }
  if (ModuleStatus::IDLE == module->m_Status)
  {
    Parser parser(module, m_ModManager);
    auto parseError = parser.Parse();
    if (parseError.has_value())
    {
      module->m_Status = ModuleStatus::INVALID;
      m_Diagnostics.push_back(parseError.value());
      return nullptr;
    }
    Checker checker(module, m_ModManager);
    auto diagnostics = checker.Check();
    m_Diagnostics.insert(m_Diagnostics.end(), diagnostics.begin(), diagnostics.end());
    module->m_Status = ModuleStatus::LOADED;
  }
  auto objectType = new type::Object();
  for (auto &bind : module->m_Exports->Store)
  {
    objectType->m_Entries[bind.first] = bind.second->m_Type;
  }
  auto moduleBind = new BindMod(importStmt->GetName(), importStmt->GetPosition(), importStmt->GetNamePosition(), m_Module->m_ID, module->m_Exports, objectType);
  SaveBind(importStmt->GetName(), moduleBind);
  return nullptr;
}

Bind *Checker::CheckExpr(Expr *expr)
{
  switch (expr->GetType())
  {
  case ExprT::Assign:
    return CheckExprAssign((AssignExpr *)expr);
  case ExprT::Call:
    return CheckExprCall((CallExpr *)expr);
  case ExprT::String:
    return CheckExprString((StringExpr *)expr);
  case ExprT::Ident:
    return CheckExprIdent((IdentExpr *)expr);
  case ExprT::FieldAcc:
    return CheckExprFieldAcc((FieldAccExpr *)expr);
  }
  return nullptr;
}

Bind *Checker::CheckExprCall(CallExpr *callExpr)
{
  // callee
  auto calleeBind = CheckExpr(callExpr->GetCallee());
  if (!calleeBind)
  {
    return nullptr;
  }
  if (type::Base::FUNCTION != calleeBind->m_Type->m_Base)
  {
    m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, callExpr->GetCalleePosition(), m_Module->m_ID, DiagnosticSeverity::ERROR, "call to non-callable object"));
    return nullptr;
  }
  auto calleeFnType = (type::Function *)calleeBind->m_Type;
  // arguments
  auto callExpressionArgs = callExpr->GetArguments();
  auto callExpressionArgsPosition = callExpr->GetArgumentsPosition();
  if (calleeFnType->m_IsVarArgs)
  {
    if (calleeFnType->m_ReqArgsCount > callExpressionArgs.size())
    {
      m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, callExpressionArgsPosition, m_Module->m_ID, DiagnosticSeverity::ERROR, ""));
      return nullptr;
    }
  }
  else if (calleeFnType->m_ReqArgsCount != callExpressionArgs.size())
  {
    m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, callExpressionArgsPosition, m_Module->m_ID, DiagnosticSeverity::ERROR, ""));
    return nullptr;
  }
  for (size_t i = 0; i < callExpressionArgs.size(); ++i)
  {
    auto argumentBind = CheckExpr(callExpressionArgs.at(i));
    if (!argumentBind)
    {
      continue;
    }
    argumentBind->m_IsUsed = true;
    if (i >= calleeFnType->m_Args.size())
    {
      continue;
    }
    if (calleeFnType->m_Args.at(i)->IsCompatibleWith(argumentBind->m_Type))
    {
      continue;
    }
    m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, argumentBind->m_Pos, m_Module->m_ID, DiagnosticSeverity::ERROR, "argument type mismatch"));
  }
  return new Bind(BindT::Expr, calleeFnType->m_RetType, m_Module->m_ID, callExpr->GetPosition());
}

Bind *Checker::CheckExprIdent(IdentExpr *identExpr)
{
  auto bind = LookupBind(identExpr->GetValue());
  if (bind)
  {
    bind->m_IsUsed = true;
    return new Bind(BindT::Expr, bind->m_Type, m_Module->m_ID, identExpr->GetPosition(), false, false, bind->m_Ref ? bind->m_Ref : bind);
  }
  m_Diagnostics.push_back(Diagnostic(Errno::NAME_ERROR, identExpr->GetPosition(), m_Module->m_ID, DiagnosticSeverity::ERROR, "undefined name"));
  return nullptr;
}

Bind *Checker::CheckExprAssign(AssignExpr *assignExpr)
{
  // assignee
  auto destBind = CheckExprIdent(assignExpr->GetAssignee());
  if (!destBind)
  {
    return nullptr;
  }
  // value
  auto valueBind = CheckExpr(assignExpr->GetValue());
  if (!valueBind)
  {
    return nullptr;
  }
  // match types
  if (!destBind->m_Type->IsCompatibleWith(valueBind->m_Type))
  {
    m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, valueBind->m_Pos, valueBind->m_ModID, DiagnosticSeverity::ERROR, "expect value of type {} but got {}"));
    return nullptr;
  }
  // TODO: update type instead of reassigning to allow single modification point
  return nullptr;
}

Bind *Checker::CheckExprFieldAcc(FieldAccExpr *fieldAccExpr)
{
  auto valueBind = CheckExpr(fieldAccExpr->GetValue());
  if (!valueBind)
  {
    return nullptr;
  }
  if (type::Base::OBJECT != valueBind->m_Type->m_Base)
  {
    m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, fieldAccExpr->GetPosition(), m_Module->m_ID, DiagnosticSeverity::ERROR, "object is not indexable"));
    return nullptr;
  }
  auto bindObjectType = (type::Object *)valueBind->m_Type;
  if (bindObjectType->m_Entries.find(fieldAccExpr->GetFieldName()->GetValue()) == bindObjectType->m_Entries.end())
  {
    std::string fieldNotFoundErrorMessage;
    switch (valueBind->m_Ref->m_BindT)
    {
    case BindT::Mod:
      fieldNotFoundErrorMessage = std::format("module '{}' has not field '{}'", ((BindMod *)valueBind->m_Ref)->m_Name, fieldAccExpr->GetFieldName()->GetValue());
      break;
    default:
      fieldNotFoundErrorMessage = std::format("object '{}' has no field '{}'", bindObjectType->Inspect(), fieldAccExpr->GetFieldName()->GetValue());
    }
    m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, fieldAccExpr->GetFieldName()->GetPosition(), m_Module->m_ID, DiagnosticSeverity::ERROR, fieldNotFoundErrorMessage));
    return nullptr;
  }
  return new Bind(BindT::Expr, bindObjectType->m_Entries.at(fieldAccExpr->GetFieldName()->GetValue()), m_Module->m_ID, fieldAccExpr->GetPosition());
}

Bind *Checker::CheckExprString(StringExpr *stringExpr)
{
  return new Bind(BindT::Expr, new type::Type(type::Base::STRING), m_Module->m_ID, stringExpr->GetPosition());
}

void Checker::EnterScope(ScopeType scopeType)
{
  m_Scopes.push_back(Scope(scopeType));
}

void Checker::LeaveScope()
{
  for (auto &bind : m_Scopes.back().m_Context.Store)
  {
    if (bind.second->m_IsUsed || bind.first.starts_with('_') || bind.second->m_IsPub || (ScopeType::GLOBAL == m_Scopes.back().m_Type && bind.first == "main"))
    {
      continue;
    }
    switch (bind.second->m_BindT)
    {
    case BindT::Expr:
    case BindT::RetVal:
      /* Values with these Bind types never get stored in the context */
      break;
    case BindT::Mod:
      m_Diagnostics.push_back(Diagnostic(Errno::UNUSED_VALUE, ((BindMod *)bind.second)->m_NamePos, bind.second->m_ModID, DiagnosticSeverity::WARN, "unused import"));
      break;
    case BindT::Var:
      m_Diagnostics.push_back(Diagnostic(Errno::UNUSED_VALUE, bind.second->m_Pos, bind.second->m_ModID, DiagnosticSeverity::WARN, "unused variable"));
      break;
    case BindT::Param:
      m_Diagnostics.push_back(Diagnostic(Errno::UNUSED_VALUE, bind.second->m_Pos, bind.second->m_ModID, DiagnosticSeverity::WARN, "unused parameter"));
      break;
    case BindT::Fun:
      m_Diagnostics.push_back(Diagnostic(Errno::UNUSED_VALUE, ((BindFun *)bind.second)->NamePosition, bind.second->m_ModID, DiagnosticSeverity::WARN, std::format("function '{}' never gets called", bind.first)));
      break;
    }
  }
  if (ScopeType::GLOBAL == m_Scopes.back().m_Type)
  {
    m_Module->m_Exports = new ModuleContext();
    for (auto &pair : m_Scopes.back().m_Context.Store)
    {
      if (pair.second->m_IsPub)
      {
        m_Module->m_Exports->Store[pair.first] = pair.second;
      }
    }
  }
  m_Scopes.pop_back();
}

Bind *Checker::LookupBind(std::string name)
{
  for (auto it = m_Scopes.rbegin(); it != m_Scopes.rend(); ++it)
  {
    auto binding = it->m_Context.Get(name);
    if (binding)
      return binding;
  }
  return nullptr;
}

void Checker::SaveBind(std::string name, Bind *bind)
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
std::string Checker::NormalizeImportPath(bool hasAtNotation, std::vector<IdentExpr *> path)
{
  std::ostringstream oss;
  if (hasAtNotation)
  {
    // TODO: prefix with ZEROLANG_HOME env variable
  }
  for (size_t i = 0; i < path.size(); ++i)
  {
    oss << path.at(i)->GetValue();
    if ((i + 1) < path.size())
    {
      oss << "/";
    }
  }
  oss << ".zr";
  return oss.str();
}
