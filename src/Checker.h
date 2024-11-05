#pragma once

#include "Ast.h"
#include "AstVisitor.h"
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
  bool IsUsed;
  ObjectSource Source;

  Object(class Type *type, ObjectSource objectSource) : Type(type), IsUsed(false), Source(objectSource) {};
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
  Checker() = default;

  void Check(AST &ast);

private:
  unsigned long ErrorsCount = 0;

  std::vector<Scope *> Scopes;

  void EnterScope(ScopeType);
  void LeaveScope();
  bool InScope(ScopeType);
  bool ExistInCurrentScope(std::string);
  Scope *GetCurrentScope();
  void DeclareFunction(std::string, Type, std::vector<Type>, bool);

  void ValidateEntryPoint();

  void *visit(BlockStatement *) override;
  void *visit(FunctionStatement *) override;
  void *visit(ReturnStatement *) override;
  void *visit(CallExpression *) override;
  void *visit(IdentifierExpression *) override;
  void *visit(StringExpression *) override;
  void *visit(IntegerExpression *) override;
};
