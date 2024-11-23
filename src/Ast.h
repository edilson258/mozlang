#pragma once

#include "AstVisitor.h"
#include "Location.h"
#include "Token.h"
#include "TypeSystem.h"

#include <optional>
#include <string>
#include <vector>

enum class AstNodeType
{
  BlockStatement,
  FunctionStatement,
  ReturnStatement,
  CallExpression,
  IdentifierExpression,
  StringExpression,
  IntegerExpression,
  BooleanExpression,
};

class AstNode
{
public:
  Location Loc;
  AstNodeType Type;

  virtual void *accept(AstVisitor *) = 0;
  virtual ~AstNode()                 = default;

protected:
  AstNode(Location loc, AstNodeType type) : Loc(loc), Type(type) {};
};

class Statement : public AstNode
{
public:
  virtual ~Statement() = default;

protected:
  Statement(Location loc, AstNodeType type) : AstNode(loc, type) {};
};

class Expression : public Statement
{
protected:
  Expression(Location loc, AstNodeType type) : Statement(loc, type) {};
};

class IdentifierExpression : public Expression
{
public:
  Token Lexeme;

  IdentifierExpression(Token lexeme) : Expression(lexeme.Loc, AstNodeType::IdentifierExpression), Lexeme(lexeme) {};

  std::string GetValue() const;
  void *accept(AstVisitor *visitor) override;
};

class StringExpression : public Expression
{
public:
  Token Lexeme;

  StringExpression(Token lexeme) : Expression(lexeme.Loc, AstNodeType::StringExpression), Lexeme(lexeme) {};

  std::string GetValue() const;
  void *accept(AstVisitor *visitor) override;
};

class IntegerExpression : public Expression
{
public:
  Token Lexeme;

  IntegerExpression(Token lexeme) : Expression(lexeme.Loc, AstNodeType::IntegerExpression), Lexeme(lexeme) {};

  long long GetValue() const;
  std::string GetRawValue() const;
  void *accept(AstVisitor *visitor) override;
};

class BooleanExpression : public Expression
{
public:
  Token Lexeme;
  bool Value;

  BooleanExpression(Token lexeme, bool value)
      : Expression(lexeme.Loc, AstNodeType::BooleanExpression), Lexeme(lexeme), Value(value) {};

  void *accept(AstVisitor *visitor) override;
};

class CallExpressionArgs
{
public:
  Location Loc;
  std::vector<Expression *> Args;

  CallExpressionArgs(Location loc, std::vector<Expression *> args) : Loc(loc), Args(args) {};
};

class CallExpression : public Expression
{
public:
  Expression *Callee;
  CallExpressionArgs Args;

  CallExpression(Location loc, Expression *callee, CallExpressionArgs args)
      : Expression(loc, AstNodeType::CallExpression), Callee(callee), Args(args) {};

  void *accept(AstVisitor *visitor) override;
};

class BlockStatement : public Statement
{
public:
  std::vector<Statement *> Statements;

  BlockStatement(Location loc, std::vector<Statement *> statements)
      : Statement(loc, AstNodeType::BlockStatement), Statements(statements) {};

  void *accept(AstVisitor *visitor) override;
};

class TypeAnnotation
{
public:
  class Type *Type;
  std::optional<Token> Lexeme;

  TypeAnnotation(class Type *type) : Type(type), Lexeme(std::nullopt) {};
  TypeAnnotation(class Type *type, Token lexeme) : Type(type), Lexeme(lexeme) {};
};

class FunctionParam
{
public:
  Location Loc;
  IdentifierExpression *Identifier;
  class TypeAnnotation TypeAnnotation;

  FunctionParam(Location loc, IdentifierExpression *identifier, class TypeAnnotation type)
      : Loc(loc), Identifier(identifier), TypeAnnotation(type) {};
};

class FunctionStatement : public Statement
{
public:
  IdentifierExpression *Identifier;
  BlockStatement Body;
  TypeAnnotation ReturnType;
  std::vector<FunctionParam> Params;

  FunctionStatement(Location loc, IdentifierExpression *identifier, BlockStatement body, TypeAnnotation returnType,
                    std::vector<FunctionParam> params)
      : Statement(loc, AstNodeType::FunctionStatement), Identifier(identifier), Body(body), ReturnType(returnType),
        Params(params) {};

  void *accept(AstVisitor *visitor) override;
};

class ReturnStatement : public Statement
{
public:
  std::optional<Expression *> Value;

  ReturnStatement(Location loc) : Statement(loc, AstNodeType::ReturnStatement), Value(std::nullopt) {};
  ReturnStatement(Location loc, Expression *value) : Statement(loc, AstNodeType::ReturnStatement), Value(value) {};

  void *accept(AstVisitor *visitor) override;
};

class AST
{
public:
  std::vector<AstNode *> Nodes;

  AST() : Nodes() {};

  std::string ToStrng();
};
