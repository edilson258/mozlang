#include "Checker.h"
#include "Ast.h"
#include "DiagnosticEngine.h"
#include "Painter.h"
#include "Span.h"
#include "TypeSystem.h"

#include <cstdlib>
#include <format>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>

void Checker::Check(AST &ast)
{
  EnterScope(ScopeType::Global);

  // Setup builtins
  GetCurrentScope()->Store["print"] = new Object(new TypeFunction(Type(BaseType::Void), {Type(BaseType::String)}, true),
                                                 ObjectSource::Declaration, Span());

  for (AstNode *node : ast.Nodes)
  {
    node->accept(this);
  }

  ValidateEntryPoint();

  LeaveScope();

  if (ErrorsCount > 0)
  {
    std::cerr << Painter::Paint(std::format("[ABORT]: Aborting due to the {} previous errors\n", ErrorsCount),
                                Color::Brown);
    std::exit(1);
  }
}

void Checker::EnterScope(ScopeType type) { Scopes.push_back(new Scope(type)); }

void Checker::LeaveScope()
{
  for (std::pair<std::string, Object *> pair : GetCurrentScope()->Store)
  {
    if (pair.second->IsUsed || pair.first.starts_with("_"))
    {
      continue;
    }

    if (ScopeType::Global == GetCurrentScope()->Type && "main" == pair.first)
    {
      continue;
    }

    ErrorsCount++;
    Diagnostic.Error(ErrorCode::UnusedName, std::format("Unused name '{}', try to prefix it with '_'", pair.first),
                     pair.second->Spn);
  }
  Scopes.pop_back();
}

bool Checker::InScope(ScopeType type)
{
  for (auto it = Scopes.rbegin(); it != Scopes.rend(); ++it)
  {
    if ((*it)->Type == type)
    {
      return true;
    }
  }
  return false;
}

Scope *Checker::GetCurrentScope() { return Scopes.at(Scopes.size() - 1); }

bool Checker::ExistInCurrentScope(std::string name)
{
  if (GetCurrentScope()->Store.find(name) == GetCurrentScope()->Store.end())
  {
    return false;
  }
  return true;
}

void Checker::ValidateEntryPoint()
{
  if (!ExistInCurrentScope("main"))
  {
    ErrorsCount++;
    std::cerr << "[ERROR]: Missing main function\n";
    return;
  }

  Object *main = GetCurrentScope()->Store["main"];

  if (BaseType::Function != main->Type->Base)
  {
    ErrorsCount++;
    std::cerr << "[ERROR]: `main` must be callable\n";
    return;
  }

  TypeFunction *mainFnType = static_cast<TypeFunction *>(main->Type);

  if (BaseType::Integer != mainFnType->ReturnType.Base)
  {
    ErrorsCount++;
    std::cerr << "[ERROR]: `main` must return `int`\n";
  }

  if (mainFnType->IsVarArgs || mainFnType->ParamTypes.size() != 0)
  {
    ErrorsCount++;
    std::cerr << "[ERROR]: `main` cannot accept any argument\n";
  }
}

void *Checker::visit(FunctionStatement *fnStmt)
{
  if (ExistInCurrentScope(fnStmt->Identifier->GetValue()))
  {
    ErrorsCount++;
    Diagnostic.Error(ErrorCode::NameAlreadyBound,
                     std::format("Name '{}' is already in use.", fnStmt->Identifier->GetValue()),
                     fnStmt->Identifier->Spn);
    return nullptr;
  }

  std::vector<Type> paramTypes;
  for (FunctionParam &param : fnStmt->Params)
  {
    if (BaseType::Void == param.TypeAnnotation.Type->Base)
    {
      ErrorsCount++;
      Diagnostic.Error(ErrorCode::InvalidTypeAnnotation, "Parameter can't be of type void.", param.Spn);
    }
    paramTypes.push_back(*param.TypeAnnotation.Type);
  }

  GetCurrentScope()->Store[fnStmt->Identifier->GetValue()] =
      new Object(new TypeFunction(*fnStmt->ReturnType.Type, paramTypes, false), ObjectSource::Declaration,
                 fnStmt->Identifier->Spn);
  EnterScope(ScopeType::Function);

  for (FunctionParam &param : fnStmt->Params)
  {
    GetCurrentScope()->Store[param.Identifier->GetValue()] =
        new Object(param.TypeAnnotation.Type, ObjectSource::Parameter, param.Identifier->Spn);
  }

  std::vector<Object *> *results = static_cast<std::vector<Object *> *>(fnStmt->Body.accept(this));

  bool returnedSomething = false;

  for (Object *result : *results)
  {
    if (ObjectSource::Expession == result->Source)
    {
      ErrorsCount++;
      Diagnostic.Error(ErrorCode::UnusedValue, "Expression results to unused value", result->Spn);
    }

    if (ObjectSource::FunctionCallResult == result->Source)
    {
      ErrorsCount++;
      Diagnostic.Error(ErrorCode::UnusedValue, "Function's return value is not used.", result->Spn);
    }

    if (ObjectSource::ReturnValue == result->Source)
    {
      returnedSomething = true;

      if (*fnStmt->ReturnType.Type != *result->Type)
      {
        ErrorsCount++;
        Diagnostic.Error(ErrorCode::TypesNoMatch,
                         std::format("Function '{}' expects value of type '{}' but returned value of type '{}'",
                                     fnStmt->Identifier->GetValue(), fnStmt->ReturnType.Type->ToString(),
                                     result->Type->ToString()),
                         result->Spn);
      }
    }
  }

  if (!returnedSomething)
  {
    if (BaseType::Void == fnStmt->ReturnType.Type->Base)
    {
      fnStmt->Body.Statements.push_back(new ReturnStatement(Span()));
    }
    else
    {
      ErrorsCount++;
      Diagnostic.Error(ErrorCode::MissingValue, "Non-void function does not return value.", fnStmt->Spn);
    }
  }

  LeaveScope();

  return nullptr;
};

void *Checker::visit(ReturnStatement *retStmt)
{
  if (!retStmt->Value.has_value())
  {
    return nullptr;
  }
  void *x = retStmt->Value.value()->accept(this);
  if (!x)
  {
    return nullptr;
  }
  Object *value = static_cast<Object *>(x);
  value->Source = ObjectSource::ReturnValue;
  return value;
}

void *Checker::visit(BlockStatement *blockStmt)
{
  std::vector<Object *> *xs = new std::vector<Object *>();

  for (unsigned long i = 0; i < blockStmt->Statements.size(); ++i)
  {
    Statement *stmt = blockStmt->Statements.at(i);
    if (stmt->Type == AstNodeType::ReturnStatement && ((blockStmt->Statements.size() - i - 1) > 0))
    {
      ErrorsCount++;
      Span spn     = blockStmt->Statements.at(i + 1)->Spn;
      spn.RangeEnd = blockStmt->Spn.RangeEnd - 1;
      Diagnostic.Error(ErrorCode::DeadCode, "Unreachable code detected.", spn);

      void *x = stmt->accept(this);
      if (x)
      {
        xs->push_back(static_cast<Object *>(x));
      }
      break;
    }

    void *x = stmt->accept(this);
    if (x)
    {
      xs->push_back(static_cast<Object *>(x));
    }
  }

  return xs;
}

void *Checker::visit(CallExpression *callExpr)
{
  void *callee = callExpr->Callee->accept(this);
  if (!callee)
  {
    return nullptr;
  }

  Object *calleeObj = static_cast<Object *>(callee);

  if (BaseType::Function != calleeObj->Type->Base)
  {
    ErrorsCount++;
    Diagnostic.Error(ErrorCode::CallNotCallable, "Callee is not callable.", calleeObj->Spn);
    return nullptr;
  }

  TypeFunction *calleeFnType = static_cast<TypeFunction *>(calleeObj->Type);

  if (callExpr->Args.Args.size() != calleeFnType->ParamTypes.size())
  {
    if (!calleeFnType->IsVarArgs)
    {
      ErrorsCount++;
      Diagnostic.Error(ErrorCode::ArgsCountNoMatch,
                       std::format("Callee expects '{}' args but got '{}'.", calleeFnType->ParamTypes.size(),
                                   callExpr->Args.Args.size()),
                       callExpr->Args.Spn);
      return nullptr;
    }

    if (callExpr->Args.Args.size() < calleeFnType->ParamTypes.size())
    {
      ErrorsCount++;
      Diagnostic.Error(ErrorCode::ArgsCountNoMatch,
                       std::format("Callee expects '{}' required args but got '{}'.", calleeFnType->ParamTypes.size(),
                                   callExpr->Args.Args.size()),
                       callExpr->Args.Spn);
      return nullptr;
    }
  }

  std::vector<Object *> argObjects;
  std::set<unsigned long> badArgPositions;

  for (unsigned long i = 0; i < callExpr->Args.Args.size(); ++i)
  {
    void *x = callExpr->Args.Args.at(i)->accept(this);
    if (!x)
    {
      badArgPositions.insert(i);
      continue;
    }
    argObjects.push_back(static_cast<Object *>(x));
  }

  for (unsigned long i = 0; i < calleeFnType->ParamTypes.size(); ++i)
  {
    if (badArgPositions.contains(i))
    {
      continue;
    }

    Type argType   = *argObjects.at(i)->Type;
    Type paramType = calleeFnType->ParamTypes.at(i);

    if (argType != paramType)
    {
      ErrorsCount++;
      Diagnostic.Error(ErrorCode::TypesNoMatch,
                       std::format("Argument of type '{}' is not assignable to param of type '{}'.", argType.ToString(),
                                   paramType.ToString()),
                       argObjects.at(i)->Spn);
    }
  }

  if (BaseType::Void == calleeFnType->ReturnType.Base)
  {
    return nullptr;
  }

  return new Object(&calleeFnType->ReturnType, ObjectSource::FunctionCallResult, callExpr->Spn);
}

void *Checker::visit(IdentifierExpression *identExpr)
{
  for (auto scopeIterator = Scopes.rbegin(); scopeIterator != Scopes.rend(); ++scopeIterator)
  {
    if ((*scopeIterator)->Store.find(identExpr->GetValue()) != (*scopeIterator)->Store.end())
    {
      (*scopeIterator)->Store[identExpr->GetValue()]->IsUsed = true;
      return (*scopeIterator)->Store[identExpr->GetValue()];
    }
  }
  ErrorsCount++;
  Diagnostic.Error(ErrorCode::UnboundName, std::format("Name '{}' is not defined.", identExpr->GetValue()),
                   identExpr->Spn);
  return nullptr;
}

void *Checker::visit(StringExpression *strExpr)
{
  return new Object(new Type(BaseType::String), ObjectSource::Expession, strExpr->Spn);
}
void *Checker::visit(IntegerExpression *intExpr)
{
  return new Object(new Type(BaseType::Integer), ObjectSource::Expession, intExpr->Spn);
}
