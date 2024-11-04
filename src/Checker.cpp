#include "Checker.h"
#include "Ast.h"
#include "TypeSystem.h"

#include <cstdlib>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>

void Checker::Check(AST &ast)
{
  EnterScope(ScopeType::Global);

  // Setup builtins
  DeclareFunction("print", Type(BaseType::Void), {Type(BaseType::String)}, true);

  for (AstNode *node : ast.Nodes)
  {
    node->accept(this);
  }

  ValidateEntryPoint();

  LeaveScope();

  if (ErrorsCount > 0)
  {
    std::cerr << "[ABORT]: Aborting due to the " << ErrorsCount << " previuos errors\n";
    std::exit(1);
  }
}

void Checker::EnterScope(ScopeType type) { Scopes.push_back(new Scope(type)); }

void Checker::LeaveScope()
{
  for (std::pair<std::string, Object *> pair : GetCurrentScope()->Store)
  {
    if (pair.second->IsUsed)
    {
      continue;
    }

    if (ScopeType::Global == GetCurrentScope()->Type && "main" == pair.first)
    {
      continue;
    }

    ErrorsCount++;
    std::cerr << "[ERROR]: Unused name " << pair.first << std::endl;
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

void Checker::DeclareFunction(std::string name, Type returnType, std::vector<Type> paramType, bool isVarArgs)
{
  GetCurrentScope()->Store[name] =
      new Object(new TypeFunction(returnType, paramType, isVarArgs), ObjectSource::Declaration);
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
    std::cerr << "[ERROR]: Name is already bound: " << fnStmt->Identifier->GetValue() << std::endl;
    return nullptr;
  }

  DeclareFunction(fnStmt->Identifier->GetValue(), *fnStmt->ReturnType.Type, {}, false);
  EnterScope(ScopeType::Function);

  std::vector<Object *> *results = static_cast<std::vector<Object *> *>(fnStmt->Body.accept(this));

  bool returnedSomething = false;

  for (Object *result : *results)
  {
    if (ObjectSource::Expession == result->Source)
    {
      ErrorsCount++;
      std::cerr << "[ERROR]: Expression results to unused value\n";
    }

    if (ObjectSource::FunctionCallResult == result->Source)
    {
      ErrorsCount++;
      std::cerr << "[ERROR]: Unused return value\n";
    }

    if (ObjectSource::ReturnValue == result->Source)
    {
      returnedSomething = true;

      if (*fnStmt->ReturnType.Type != *result->Type)
      {
        ErrorsCount++;
        std::cerr << "[ERROR]: Return type miss match\n";
      }
    }
  }

  if (!returnedSomething)
  {
    if (BaseType::Void == fnStmt->ReturnType.Type->Base)
    {
      fnStmt->Body.Statements.push_back(new ReturnStatement(Token()));
    }
    else
    {
      ErrorsCount++;
      std::cerr << "[ERROR]: Non-void function does not return a value\n";
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
      std::cerr << "[ERROR]: Dead code detected\n";
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
    std::cerr << "[ERROR]: Object has no callable signature\n";
    return nullptr;
  }

  TypeFunction *calleeFnType = static_cast<TypeFunction *>(calleeObj->Type);

  if (callExpr->Args.size() != calleeFnType->ParamTypes.size())
  {
    if (!calleeFnType->IsVarArgs)
    {
      ErrorsCount++;
      std::cerr << "[ERROR]: Args count miss match\n";
      return nullptr;
    }

    if (callExpr->Args.size() < calleeFnType->ParamTypes.size())
    {
      ErrorsCount++;
      std::cerr << "[ERROR]: Args count miss match, missing required ones\n";
      return nullptr;
    }
  }

  std::vector<Object *> argObjects;
  std::set<unsigned long> badArgPositions;

  for (unsigned long i = 0; i < callExpr->Args.size(); ++i)
  {
    void *x = callExpr->Args.at(i)->accept(this);
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

    if (*argObjects.at(i)->Type != calleeFnType->ParamTypes.at(i))
    {
      ErrorsCount++;
      std::cerr << "[ERROR]: Argument type does not match\n";
    }
  }

  if (BaseType::Void == calleeFnType->ReturnType.Base)
  {
    return nullptr;
  }

  return new Object(&calleeFnType->ReturnType, ObjectSource::FunctionCallResult);
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
  std::cerr << "[ERROR]: Unbound name: " << identExpr->GetValue() << std::endl;
  return nullptr;
}

void *Checker::visit(StringExpression *) { return new Object(new Type(BaseType::String), ObjectSource::Expession); }
void *Checker::visit(IntegerExpression *) { return new Object(new Type(BaseType::Integer), ObjectSource::Expession); }
