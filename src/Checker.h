#pragma once

#include "Ast.h"
#include "AstVisitor.h"
#include "DiagnosticEngine.h"
#include "Location.h"
#include "TypeSystem.h"

#include <string>
#include <unordered_map>
#include <vector>

/**
 *  Tell us how an object/value was created.
 *
 */
enum class ObjectSource
{
  // The object is a result of a return statement
  //
  // Example:
  //
  //    fn main(): int {
  //      ...
  //      return 69; // The integer `69` has `ReturnValue` source.
  //      ---------
  //    }
  //
  ReturnValue,

  // The object is a result of
  //
  //    literal expression
  //    binary expression
  //    ...
  //
  Expession,

  // The object was returned by a function call expression
  //
  FunctionCallResult,

  // The object was declared by the user
  //
  //    Function declaration,
  //    Variable declaration,
  //    ...
  //
  Declaration,

  Parameter,
};

class Object
{
public:
  Type *Type;
  ObjectSource Source;
  Location Loc;
  bool IsUsed;

  Object(class Type *type, ObjectSource objectSource, Location loc, bool isUsed = false)
      : Type(type), Source(objectSource), Loc(loc), IsUsed(isUsed) {};
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
  Checker(DiagnosticEngine &diagnostic) : Diagnostic(diagnostic) {};

  void Check(AST &ast);

private:
  DiagnosticEngine &Diagnostic;

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
