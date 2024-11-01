#pragma once

#include "AstVisitor.h"
#include "Token.h"

#include <vector>

enum class AstNodeType
{
  BlockStatement,
  FunctionStatement,
  CallExpression,
  IdentifierExpression,
  StringExpression,
};

class AstNode
{
public:
  AstNodeType Type;

  virtual void *accept(AstVisitor *) = 0;

  virtual ~AstNode() = default;

protected:
  AstNode(AstNodeType type) : Type(type) {};
};

class Statement : public AstNode
{
public:
  virtual ~Statement() = default;

protected:
  Statement(AstNodeType type) : AstNode(type) {};
};

class Expression : public Statement
{
protected:
  Expression(AstNodeType type) : Statement(type) {};
};

class IdentifierExpression : public Expression
{
public:
  Token Identifier;

  IdentifierExpression(Token identifier) : Expression(AstNodeType::IdentifierExpression), Identifier(identifier) {};

  std::string GetValue() const;
  void *accept(AstVisitor *visitor) override;
};

class StringExpression : public Expression
{
public:
  Token String;

  StringExpression(Token string) : Expression(AstNodeType::StringExpression), String(string) {};

  std::string GetValue() const;
  void *accept(AstVisitor *visitor) override;
};

class CallExpression : public Expression
{
public:
  Expression *Callee;
  std::vector<Expression *> Args;

  CallExpression(Expression *callee, std::vector<Expression *> args)
      : Expression(AstNodeType::CallExpression), Callee(callee), Args(args) {};

  void *accept(AstVisitor *visitor) override;
};

class BlockStatement : public Statement
{
public:
  std::vector<Statement *> Statements;

  BlockStatement(std::vector<Statement *> statements)
      : Statement(AstNodeType::BlockStatement), Statements(statements) {};

  void *accept(AstVisitor *visitor) override;
};

class FunctionStatement : public Statement
{
public:
  IdentifierExpression *Identifier;
  BlockStatement Body;

  FunctionStatement(IdentifierExpression *identifier, BlockStatement body)
      : Statement(AstNodeType::FunctionStatement), Identifier(identifier), Body(body) {};

  void *accept(AstVisitor *visitor) override;
};

class AST
{
public:
  std::vector<AstNode *> Nodes;

  AST() : Nodes() {};

  std::string ToStrng();
};
