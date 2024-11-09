#pragma once

#include "AstVisitor.h"
#include "Span.h"
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
};

class AstNode
{
public:
  Span Spn;
  AstNodeType Type;

  virtual void *accept(AstVisitor *) = 0;
  virtual ~AstNode()                 = default;

protected:
  AstNode(Span spn, AstNodeType type) : Spn(spn), Type(type) {};
};

class Statement : public AstNode
{
public:
  virtual ~Statement() = default;

protected:
  Statement(Span spn, AstNodeType type) : AstNode(spn, type) {};
};

class Expression : public Statement
{
protected:
  Expression(Span spn, AstNodeType type) : Statement(spn, type) {};
};

class IdentifierExpression : public Expression
{
public:
  Token Lexeme;

  IdentifierExpression(Token lexeme) : Expression(lexeme.Spn, AstNodeType::IdentifierExpression), Lexeme(lexeme) {};

  std::string GetValue() const;
  void *accept(AstVisitor *visitor) override;
};

class StringExpression : public Expression
{
public:
  Token Lexeme;

  StringExpression(Token lexeme) : Expression(lexeme.Spn, AstNodeType::StringExpression), Lexeme(lexeme) {};

  std::string GetValue() const;
  void *accept(AstVisitor *visitor) override;
};

class IntegerExpression : public Expression
{
public:
  Token Lexeme;

  IntegerExpression(Token lexeme) : Expression(lexeme.Spn, AstNodeType::IntegerExpression), Lexeme(lexeme) {};

  long long GetValue() const;
  std::string GetRawValue() const;
  void *accept(AstVisitor *visitor) override;
};

class CallExpressionArgs
{
public:
  Span Spn;
  std::vector<Expression *> Args;

  CallExpressionArgs(Span spn, std::vector<Expression *> args) : Spn(spn), Args(args) {};
};

class CallExpression : public Expression
{
public:
  Expression *Callee;
  CallExpressionArgs Args;

  CallExpression(Span spn, Expression *callee, CallExpressionArgs args)
      : Expression(spn, AstNodeType::CallExpression), Callee(callee), Args(args) {};

  void *accept(AstVisitor *visitor) override;
};

class BlockStatement : public Statement
{
public:
  std::vector<Statement *> Statements;

  BlockStatement(Span spn, std::vector<Statement *> statements)
      : Statement(spn, AstNodeType::BlockStatement), Statements(statements) {};

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
  Span Spn;
  IdentifierExpression *Identifier;
  class TypeAnnotation TypeAnnotation;

  FunctionParam(Span spn, IdentifierExpression *identifier, class TypeAnnotation type)
      : Spn(spn), Identifier(identifier), TypeAnnotation(type) {};
};

class FunctionStatement : public Statement
{
public:
  IdentifierExpression *Identifier;
  BlockStatement Body;
  TypeAnnotation ReturnType;
  std::vector<FunctionParam> Params;

  FunctionStatement(Span spn, IdentifierExpression *identifier, BlockStatement body, TypeAnnotation returnType,
                    std::vector<FunctionParam> params)
      : Statement(spn, AstNodeType::FunctionStatement), Identifier(identifier), Body(body), ReturnType(returnType),
        Params(params) {};

  void *accept(AstVisitor *visitor) override;
};

class ReturnStatement : public Statement
{
public:
  std::optional<Expression *> Value;

  ReturnStatement(Span spn) : Statement(spn, AstNodeType::ReturnStatement), Value(std::nullopt) {};
  ReturnStatement(Span spn, Expression *value) : Statement(spn, AstNodeType::ReturnStatement), Value(value) {};

  void *accept(AstVisitor *visitor) override;
};

class AST
{
public:
  std::vector<AstNode *> Nodes;

  AST() : Nodes() {};

  std::string ToStrng();
};
