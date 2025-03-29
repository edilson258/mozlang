#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "ast.h"
#include "checker.h"
#include "context.h"
#include "diagnostic.h"
#include "error.h"
#include "module.h"
#include "parser.h"
#include "pointer.h"
#include "token.h"
#include "type.h"

std::vector<Diagnostic> Checker::Check()
{
  EnterScope(ScopeType::GLOBAL);
  for (auto statement : m_Module->m_AST->m_Program)
  {
    auto bind = CheckStmt(statement);
    if (bind && (!bind->m_IsUsed && bind->m_Type->IsSomething() && !bind->IsError()))
    {
      m_Diagnostics.push_back(Diagnostic(Errno::UNUSED_VALUE, bind->m_Pos, bind->m_ModID, DiagnosticSeverity::WARN, "expression results to unused value"));
    }
  }
  LeaveScope();
  return std::move(m_Diagnostics);
}

Ptr<Bind> Checker::CheckStmt(Ptr<Stmt> stmt)
{
  switch (stmt->GetType())
  {
  case StmtT::Fun:
    return CheckStmtFun(CastPtr<FunStmt>(stmt));
  case StmtT::Import:
    return CheckStmtImport(CastPtr<ImportStmt>(stmt));
  case StmtT::Let:
    return CheckStmtLet(CastPtr<LetStmt>(stmt));
  case StmtT::Block:
    return CheckStmtBlock(CastPtr<BlockStmt>(stmt));
  case StmtT::Ret:
    return CheckStmtRet(CastPtr<RetStmt>(stmt));
  case StmtT::Expr:
    return CheckExpr(CastPtr<Expr>(stmt));
  }
  return nullptr;
}

Ptr<Bind> Checker::CheckStmtFun(Ptr<FunStmt> funStmt)
{
  // 1. check name conflits
  FunSign sign = funStmt->GetSign();
  auto bindWithSameName = m_Scopes.back().m_Context.Get(sign.GetName());
  if (bindWithSameName)
  {
    DiagnosticReference reference(Errno::OK, bindWithSameName->m_ModID, bindWithSameName->m_Pos, "name used here");
    m_Diagnostics.push_back(Diagnostic(Errno::NAME_ERROR, sign.GetNamePos(), m_Module->m_ID, DiagnosticSeverity::ERROR, std::format("name '{}' is alredy used", sign.GetName()), reference));
    return nullptr;
  }

  // 2. function not allowed inside another functions
  if (IsWithinScope(ScopeType::FUNCTION))
  {
    m_Diagnostics.push_back(Diagnostic(Errno::SYNTAX_ERROR, sign.GetNamePos(), m_Module->m_ID, DiagnosticSeverity::ERROR, "cannot declare a function inside another function"));
    // Save error bind with same name as placeholder to avoid ghost errors through error propagation
    SaveBind(sign.GetName(), Bind::MakeError(m_Module->m_ID, sign.GetNamePos()));
    return nullptr;
  }

  // 3. build function type
  std::vector<Ptr<type::Type>> funArgsTypes;
  for (auto &param : sign.GetParams())
  {
    funArgsTypes.push_back(param.GetAstType()->GetType());
  }
  auto expectRetType = sign.GetRetType() ? sign.GetRetType()->GetType() : MakePtr(type::Type(type::Base::VOID));
  auto functionType = MakePtr(type::Function(sign.GetParams().size(), std::move(funArgsTypes), expectRetType, sign.IsVarArgs()));
  auto functionBind = MakePtr(BindFun(sign.GetPos(), sign.GetNamePos(), sign.GetParamsPos(), functionType, m_Module->m_ID, false, sign.IsPub()));
  SaveBind(sign.GetName(), functionBind);

  EnterScope(ScopeType::FUNCTION);

  // 4. save params binds inside of the new function scope
  for (auto &param : sign.GetParams())
  {
    if (m_Scopes.back().m_Context.Get(param.GetName()))
    {
      DiagnosticReference reference(Errno::OK, m_Scopes.back().m_Context.Get(param.GetName())->m_ModID, m_Scopes.back().m_Context.Get(param.GetName())->m_Pos, "first used here");
      m_Diagnostics.push_back(Diagnostic(Errno::NAME_ERROR, param.GetNamePos(), m_Module->m_ID, DiagnosticSeverity::ERROR, std::format("duplicated param name '{}'", param.GetName()), reference));
      continue;
    }
    auto paramType = param.GetAstType()->GetType();
    SaveBind(param.GetName(), MakePtr(Bind(BindT::Param, paramType, m_Module->m_ID, param.GetNamePos())));
  }

  // 5. check body if available
  auto body = funStmt->GetBody();
  if (!body)
  {
    // if is no body then is a simple declaration, eg. `pub fun println(...): void;`
    m_Scopes.pop_back();
    return nullptr;
  }

  // 6. ensure consistency between expected and returned type
  auto blockRetBind = CheckStmtBlock(body);
  if (blockRetBind)
  {
    auto foundRetType = blockRetBind->m_Type;
    if (expectRetType->IsVoid() && !foundRetType->IsUnit())
    {
      m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, blockRetBind->m_Pos, m_Module->m_ID, DiagnosticSeverity::ERROR, "void function does not accept return value"));
    }
    else if (foundRetType->Isknown() && !expectRetType->IsCompatWith(foundRetType))
    {
      auto expected = expectRetType->Inspect();
      auto provided = foundRetType->Inspect();
      DiagnosticReference reference(Errno::OK, m_Module->m_ID, sign.GetRetType()->GetPos(), std::format("expect '{}' due to here", expected));
      m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, blockRetBind->m_Pos, m_Module->m_ID, DiagnosticSeverity::ERROR, std::format("return type mismatch, expect '{}' but got '{}'", expected, provided), reference));
    }
  }
  else if (!expectRetType->IsVoid())
  {
    m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, sign.GetRetType()->GetPos(), m_Module->m_ID, DiagnosticSeverity::ERROR, "missing return value for non-void function"));
  }

  LeaveScope();
  return nullptr;
}

Ptr<Bind> Checker::CheckStmtRet(Ptr<RetStmt> retStmt)
{
  if (!IsWithinScope(ScopeType::FUNCTION) && retStmt->IsExplicity())
  {
    m_Diagnostics.push_back(Diagnostic(Errno::SYNTAX_ERROR, retStmt->GetPos(), m_Module->m_ID, DiagnosticSeverity::ERROR, "cannot return outside a function"));
    return nullptr;
  }
  auto returnBind = MakePtr(Bind(BindT::RetVal, MakePtr(type::Type(type::Base::UNIT)), m_Module->m_ID, retStmt->GetPos(), true));
  auto val = retStmt->GetValue();
  if (val)
  {
    auto valBind = CheckExpr(val);
    returnBind->m_Type = valBind->m_Type;
    returnBind->m_Ref = valBind->m_Ref;
  }
  return returnBind;
}

Ptr<Bind> Checker::CheckStmtBlock(Ptr<BlockStmt> blockStmt)
{
  std::vector<Ptr<Bind>> statementsBinds = {};
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
      Position position = statements.at(i + 1)->GetPos();
      position.m_End = blockStmt->GetPos().m_End - 1;
      m_Diagnostics.push_back(Diagnostic(Errno::DEAD_CODE, position, m_Module->m_ID, DiagnosticSeverity::WARN, "unreachable code detected"));
      break;
    }
  }

  Ptr<Bind> returnBind = nullptr;
  for (auto &bind : statementsBinds)
  {
    if (BindT::RetVal == bind->m_BindT)
    {
      returnBind = bind;
      continue;
    }
    if (!bind->m_IsUsed && bind->m_Type->IsSomething() && !bind->IsError())
    {
      m_Diagnostics.push_back(Diagnostic(Errno::UNUSED_VALUE, bind->m_Pos, m_Module->m_ID, DiagnosticSeverity::WARN, "expression results to unused value"));
    }
  }
  return returnBind;
}

Ptr<Bind> Checker::CheckStmtLet(Ptr<LetStmt> letStmt)
{
  // 1. name should be new
  auto bindWithSameName = m_Scopes.back().m_Context.Get(letStmt->GetName());
  if (bindWithSameName)
  {
    DiagnosticReference reference(Errno::OK, bindWithSameName->m_ModID, bindWithSameName->m_Pos, "name used here");
    m_Diagnostics.push_back(Diagnostic(Errno::NAME_ERROR, letStmt->GetNamePos(), m_Module->m_ID, DiagnosticSeverity::ERROR, std::format("name '{}' already used", letStmt->GetName()), reference));
    return nullptr;
  }

  SaveBind(letStmt->GetName(), Bind::MakeError(m_Module->m_ID, letStmt->GetNamePos()));

  // 2. should have aither type annotation or an init value
  if (!letStmt->GetAstType() && !letStmt->GetInit())
  {
    m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, letStmt->GetNamePos(), m_Module->m_ID, DiagnosticSeverity::ERROR, "unable to infer variable type, initialize or annotate expected type"));
    return nullptr;
  }

  // 3. ensure consistency between annotated type and type infered from init value if provided
  Ptr<Bind> ref = nullptr;
  Ptr<type::Type> letAnnotType = letStmt->GetAstType() ? letStmt->GetAstType()->GetType() : nullptr;
  if (letStmt->GetInit())
  {
    auto initBind = CheckExpr(letStmt->GetInit());
    if (initBind->IsError())
    {
      return nullptr;
    }
    if (letAnnotType && !letAnnotType->IsCompatWith(initBind->m_Type))
    {
      auto expected = letAnnotType->Inspect();
      auto provided = initBind->m_Type->Inspect();
      DiagnosticReference reference(Errno::OK, m_Module->m_ID, letStmt->GetAstType()->GetPos(), std::format("expect '{}' due to here", expected));
      m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, initBind->m_Pos, m_Module->m_ID, DiagnosticSeverity::ERROR, std::format("expect value of type '{}' but got '{}'", expected, provided), reference));
      return nullptr;
    }
    ref = initBind->m_Ref;
    letAnnotType = initBind->m_Type;
  }
  SaveBind(letStmt->GetName(), MakePtr(Bind(BindT::Var, letAnnotType, m_Module->m_ID, letStmt->GetNamePos(), false, letStmt->IsPub(), ref)));
  return nullptr;
}

Ptr<Bind> Checker::CheckStmtImport(Ptr<ImportStmt> importStmt)
{
  auto bindWithSameName = m_Scopes.back().m_Context.Get(importStmt->GetName());
  if (bindWithSameName)
  {
    DiagnosticReference reference(Errno::OK, bindWithSameName->m_ModID, bindWithSameName->m_Pos, "name used here");
    m_Diagnostics.push_back(Diagnostic(Errno::NAME_ERROR, importStmt->GetNamePos(), m_Module->m_ID, DiagnosticSeverity::ERROR, "name already used", reference));
    return nullptr;
  }

  // is just a placeholder to avoid ghost errors propagation in case of module load fail
  SaveBind(importStmt->GetName(), Bind::MakeError(m_Module->m_ID, importStmt->GetNamePos()));

  auto loadRes = m_ModManager.Load(NormalizeImportPath(importStmt->hasAtNotation(), importStmt->GetPath()));
  if (loadRes.is_err())
  {
    m_Diagnostics.push_back(Diagnostic(Errno::NAME_ERROR, importStmt->GetNamePos(), m_Module->m_ID, DiagnosticSeverity::ERROR, "failed to import module"));
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
  auto objectType = MakePtr(type::Object());
  for (auto &bind : module->m_Exports->Store)
  {
    objectType->m_Entries[bind.first] = bind.second->m_Type;
  }
  auto moduleBind = MakePtr(BindMod(importStmt->GetName(), importStmt->GetPos(), importStmt->GetNamePos(), m_Module->m_ID, module->m_Exports, objectType));
  SaveBind(importStmt->GetName(), moduleBind);
  return nullptr;
}

Ptr<Bind> Checker::CheckExpr(Ptr<Expr> expr)
{
  switch (expr->GetType())
  {
  case ExprT::Assign:
    return CheckExprAssign(CastPtr<AssignExpr>(expr));
  case ExprT::Call:
    return CheckExprCall(CastPtr<CallExpr>(expr));
  case ExprT::String:
    return CheckExprString(CastPtr<StringExpr>(expr));
  case ExprT::Number:
    return CheckExprNumber(CastPtr<NumberExpr>(expr));
  case ExprT::Ident:
    return CheckExprIdent(CastPtr<IdentExpr>(expr));
  case ExprT::FieldAcc:
    return CheckExprFieldAcc(CastPtr<FieldAccExpr>(expr));
  }
  return nullptr;
}

Ptr<Bind> Checker::CheckExprCall(Ptr<CallExpr> callExpr)
{
  // callee
  auto calleeBind = CheckExpr(callExpr->GetCallee());
  if (calleeBind->IsError())
  {
    calleeBind->m_Pos = calleeBind->m_Pos.MergeWith(callExpr->GetArgsPos());
    return calleeBind;
  }
  if (type::Base::FUNCTION != calleeBind->m_Type->m_Base)
  {
    m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, callExpr->GetCalleePos(), m_Module->m_ID, DiagnosticSeverity::ERROR, "call to non-callable object"));
    return Bind::MakeError(m_Module->m_ID, callExpr->GetCalleePos());
  }
  auto calleeFnType = CastPtr<type::Function>(calleeBind->m_Type);
  // arguments
  auto callExpressionArgs = callExpr->GetArgs();
  auto callExpressionArgsPosition = callExpr->GetArgsPos();
  if (calleeFnType->m_IsVarArgs)
  {
    if (calleeFnType->m_ReqArgsCount > callExpressionArgs.size())
    {
      m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, callExpressionArgsPosition, m_Module->m_ID, DiagnosticSeverity::ERROR, std::format("expect '{}' required args but got '{}'", calleeFnType->m_ReqArgsCount, callExpressionArgs.size())));
      return Bind::MakeError(m_Module->m_ID, callExpressionArgsPosition);
    }
  }
  else if (calleeFnType->m_ReqArgsCount != callExpressionArgs.size())
  {
    m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, callExpressionArgsPosition, m_Module->m_ID, DiagnosticSeverity::ERROR, std::format("expect '{}' required args but got '{}'", calleeFnType->m_ReqArgsCount, callExpressionArgs.size())));
    return Bind::MakeError(m_Module->m_ID, callExpressionArgsPosition);
  }
  for (size_t i = 0; i < callExpressionArgs.size(); ++i)
  {
    auto argumentBind = CheckExpr(callExpressionArgs.at(i));
    if (argumentBind->IsError() || i >= calleeFnType->m_Args.size() || calleeFnType->m_Args.at(i)->IsCompatWith(argumentBind->m_Type))
    {
      continue;
    }
    auto expect = calleeFnType->m_Args.at(i)->Inspect();
    auto found = argumentBind->m_Type->Inspect();
    m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, argumentBind->m_Pos, m_Module->m_ID, DiagnosticSeverity::ERROR, std::format("expect argument of type '{}' but got '{}'", expect, found)));
  }
  return MakePtr(Bind(BindT::Expr, calleeFnType->m_RetType, m_Module->m_ID, callExpr->GetPos()));
}

Ptr<Bind> Checker::CheckExprIdent(Ptr<IdentExpr> identExpr)
{
  auto bind = LookupBind(identExpr->GetValue());
  if (bind)
  {
    bind->m_IsUsed = true;
    return MakePtr(Bind(BindT::Expr, bind->m_Type, m_Module->m_ID, identExpr->GetPos(), false, false, bind->m_Ref ? bind->m_Ref : bind));
  }
  m_Diagnostics.push_back(Diagnostic(Errno::NAME_ERROR, identExpr->GetPos(), m_Module->m_ID, DiagnosticSeverity::ERROR, std::format("undefined name '{}'", identExpr->GetValue())));
  return Bind::MakeError(m_Module->m_ID, identExpr->GetPos());
}

Ptr<Bind> Checker::CheckExprAssign(Ptr<AssignExpr> assignExpr)
{
  // assignee
  auto destBind = CheckExprIdent(assignExpr->GetDest());
  if (destBind->IsError())
  {
    return destBind;
  }
  // value
  auto valueBind = CheckExpr(assignExpr->GetValue());
  if (valueBind->IsError())
  {
    return valueBind;
  }
  // match types
  if (!destBind->m_Type->IsCompatWith(valueBind->m_Type))
  {
    auto expect = destBind->m_Type->Inspect();
    auto found = valueBind->m_Type->Inspect();
    m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, valueBind->m_Pos, valueBind->m_ModID, DiagnosticSeverity::ERROR, std::format("expect value of type '{}' but got '{}'", expect, found)));
    return Bind::MakeError(m_Module->m_ID, assignExpr->GetValue()->GetPos());
  }
  valueBind->m_IsUsed = true;
  valueBind->m_Pos = assignExpr->GetPos();
  return valueBind;
}

Ptr<Bind> Checker::CheckExprFieldAcc(Ptr<FieldAccExpr> fieldAccExpr)
{
  auto valueBind = CheckExpr(fieldAccExpr->GetValue());
  if (valueBind->IsError())
  {
    valueBind->m_Pos = valueBind->m_Pos.MergeWith(fieldAccExpr->GetFieldName()->GetPos());
    return valueBind;
  }
  if (type::Base::OBJECT != valueBind->m_Type->m_Base)
  {
    m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, fieldAccExpr->GetValue()->GetPos(), m_Module->m_ID, DiagnosticSeverity::ERROR, "object is not indexable"));
    return Bind::MakeError(m_Module->m_ID, fieldAccExpr->GetPos());
  }
  auto bindObjType = CastPtr<type::Object>(valueBind->m_Type);
  if (bindObjType->m_Entries.find(fieldAccExpr->GetFieldName()->GetValue()) == bindObjType->m_Entries.end())
  {
    std::string fieldNotFoundErrorMessage;
    switch (valueBind->m_Ref->m_BindT)
    {
    case BindT::Mod:
      fieldNotFoundErrorMessage = std::format("module '{}' has not field '{}'", (CastPtr<BindMod>(valueBind->m_Ref))->m_Name, fieldAccExpr->GetFieldName()->GetValue());
      break;
    default:
      fieldNotFoundErrorMessage = std::format("object '{}' has no field '{}'", bindObjType->Inspect(), fieldAccExpr->GetFieldName()->GetValue());
    }
    m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, fieldAccExpr->GetFieldName()->GetPos(), m_Module->m_ID, DiagnosticSeverity::ERROR, fieldNotFoundErrorMessage));
    return Bind::MakeError(m_Module->m_ID, fieldAccExpr->GetFieldName()->GetPos());
  }
  return MakePtr(Bind(BindT::Expr, bindObjType->m_Entries.at(fieldAccExpr->GetFieldName()->GetValue()), m_Module->m_ID, fieldAccExpr->GetPos()));
}

Ptr<Bind> Checker::CheckExprString(Ptr<StringExpr> stringExpr)
{
  return MakePtr(Bind(BindT::Expr, MakePtr(type::Type(type::Base::STRING)), m_Module->m_ID, stringExpr->GetPos()));
}

Ptr<Bind> Checker::CheckExprNumber(Ptr<NumberExpr> numExpr)
{
  if (numExpr->IsFloat())
  {
    return CheckExprNumberFloat(numExpr);
  }
  if (NumberBase::Bin == numExpr->GetBase())
  {
    return CheckExprIntegerAsBinary(numExpr->GetRaw().substr(2), numExpr->GetPos());
  }
  uint64_t value;
  try
  {
    value = std::stoull(numExpr->GetRaw(), nullptr, static_cast<int>(numExpr->GetBase()));
  }
  catch (std::invalid_argument &)
  {
    m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, numExpr->GetPos(), m_Module->m_ID, DiagnosticSeverity::ERROR, "invalid number"));
    return Bind::MakeError(m_Module->m_ID, numExpr->GetPos());
  }
  catch (std::out_of_range &)
  {
    m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, numExpr->GetPos(), m_Module->m_ID, DiagnosticSeverity::ERROR, "number out of range"));
    return Bind::MakeError(m_Module->m_ID, numExpr->GetPos());
  }
  std::string bits;
  while (value > 0)
  {
    bits = (value % 2 ? '1' : '0') + bits;
    value /= 2;
  }
  return CheckExprIntegerAsBinary(bits, numExpr->GetPos());
}

Ptr<Bind> Checker::CheckExprNumberFloat(Ptr<NumberExpr> floatExpr)
{
  auto raw = floatExpr->GetRaw();
  try
  {
    (void)std::stod(raw);
  }
  catch (std::invalid_argument &)
  {
    m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, floatExpr->GetPos(), m_Module->m_ID, DiagnosticSeverity::ERROR, "invalid float number"));
    return Bind::MakeError(m_Module->m_ID, floatExpr->GetPos());
  }
  catch (std::out_of_range &)
  {
    m_Diagnostics.push_back(Diagnostic(Errno::TYPE_ERROR, floatExpr->GetPos(), m_Module->m_ID, DiagnosticSeverity::ERROR, "float number out of range"));
    return Bind::MakeError(m_Module->m_ID, floatExpr->GetPos());
  }
  return MakePtr(Bind(BindT::Expr, MakePtr(type::Type(type::Base::Float)), m_Module->m_ID, floatExpr->GetPos()));
}

Ptr<Bind> Checker::CheckExprIntegerAsBinary(std::string xs, Position pos)
{
  auto firstActiveBitPos = xs.find('1');
  if (firstActiveBitPos == std::string::npos)
  {
    return MakePtr(Bind(BindT::Expr, MakePtr(type::IntRange(false, 0)), m_Module->m_ID, pos));
  }
  auto bytes = (xs.size() - firstActiveBitPos + 7) / 8;
  return MakePtr(Bind(BindT::Expr, MakePtr(type::IntRange(false, bytes)), m_Module->m_ID, pos));
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
    case BindT::Error:
    case BindT::Expr:
    case BindT::RetVal:
      break;
    case BindT::Mod:
      m_Diagnostics.push_back(Diagnostic(Errno::UNUSED_VALUE, (CastPtr<BindMod>(bind.second))->m_NamePos, bind.second->m_ModID, DiagnosticSeverity::WARN, "unused import"));
      break;
    case BindT::Var:
      m_Diagnostics.push_back(Diagnostic(Errno::UNUSED_VALUE, bind.second->m_Pos, bind.second->m_ModID, DiagnosticSeverity::WARN, "unused variable"));
      break;
    case BindT::Param:
      m_Diagnostics.push_back(Diagnostic(Errno::UNUSED_VALUE, bind.second->m_Pos, bind.second->m_ModID, DiagnosticSeverity::WARN, "unused parameter"));
      break;
    case BindT::Fun:
      m_Diagnostics.push_back(Diagnostic(Errno::UNUSED_VALUE, (CastPtr<BindFun>(bind.second))->NamePosition, bind.second->m_ModID, DiagnosticSeverity::WARN, std::format("function '{}' never gets called", bind.first)));
      break;
    }
  }
  if (ScopeType::GLOBAL == m_Scopes.back().m_Type)
  {
    m_Module->m_Exports = MakePtr(ModuleContext());
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

Ptr<Bind> Checker::LookupBind(std::string name)
{
  for (auto it = m_Scopes.rbegin(); it != m_Scopes.rend(); ++it)
  {
    auto binding = it->m_Context.Get(name);
    if (binding)
      return binding;
  }
  return nullptr;
}

void Checker::SaveBind(std::string name, Ptr<Bind> bind)
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
std::string Checker::NormalizeImportPath(bool hasAtNotation, std::vector<Ptr<IdentExpr>> path)
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
