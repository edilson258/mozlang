#pragma once

#include "Ast.h"
#include "AstVisitor.h"
#include "DiagnosticEngine.h"
#include "Location.h"
#include "TypeSystem.h"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

enum class ObjectSource
{
  ReturnValue,
  Expession,
  Declaration,
  Parameter,
  CallResult,
};

class Object
{
public:
  Type *Typ;
  ObjectSource Source;
  bool IsUsed = false;
  Location Loc;

  Object(Type *type, ObjectSource source, Location loc) : Typ(type), Source(source), Loc(loc) {};
};

enum class ScopeType
{
  Global,
  Function,
};

class Scope
{
public:
  ScopeType Type;
  std::unordered_map<std::string, Object *> Store;

  Scope(ScopeType type) : Type(type) {};
};

class Checker : AstVisitor
{
public:
  Checker(std::filesystem::path &filePath, std::string &fileContent)
      : Diagnostic(DiagnosticEngine(filePath, fileContent)) {};

  void Check(AST &ast);

private:
  DiagnosticEngine Diagnostic;

  std::vector<Scope *> Scopes;

  void EnterScope(ScopeType);
  void LeaveScope();
  bool InScope(ScopeType);
  bool ExistInCurrentScope(std::string);
  Scope *GetCurrentScope();

  void ValidateEntryPoint();

  void *visit(BlockStatement *) override;
  void *visit(FunctionStatement *) override;
  void *visit(ReturnStatement *) override;
  void *visit(CallExpression *) override;
  void *visit(IdentifierExpression *) override;
  void *visit(StringExpression *) override;
  void *visit(IntegerExpression *) override;
  void *visit(BooleanExpression *) override;
};
