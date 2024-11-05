#pragma once

#include "AstVisitor.h"
#include "Token.h"
#include "TypeSystem.h"

#include <filesystem>
#include <optional>
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
  Token Lexeme;

  IdentifierExpression(Token lexeme) : Expression(AstNodeType::IdentifierExpression), Lexeme(lexeme) {};

  std::string GetValue() const;
  void *accept(AstVisitor *visitor) override;
};

class StringExpression : public Expression
{
public:
  Token Lexeme;

  StringExpression(Token lexeme) : Expression(AstNodeType::StringExpression), Lexeme(lexeme) {};

  std::string GetValue() const;
  void *accept(AstVisitor *visitor) override;
};

class IntegerExpression : public Expression
{
public:
  Token Lexeme;

  IntegerExpression(Token lexeme) : Expression(AstNodeType::IntegerExpression), Lexeme(lexeme) {};

  long long GetValue() const;
  std::string GetRawValue() const;
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

class TypeAnnotation
{
public:
  Type *Type;
  std::optional<Token> Lexeme;

  TypeAnnotation(class Type *type) : Type(type), Lexeme(std::nullopt) {};
  TypeAnnotation(class Type *type, Token lexeme) : Type(type), Lexeme(lexeme) {};
};

class FunctionParam
{
public:
  IdentifierExpression *Identifier;
  TypeAnnotation TypeAnnotation;

  FunctionParam(IdentifierExpression *identifier, class TypeAnnotation type)
      : Identifier(identifier), TypeAnnotation(type) {};
};

class FunctionStatement : public Statement
{
public:
  IdentifierExpression *Identifier;
  BlockStatement Body;
  TypeAnnotation ReturnType;
  std::vector<FunctionParam> Params;

  FunctionStatement(IdentifierExpression *identifier, BlockStatement body, TypeAnnotation returnType,
                    std::vector<FunctionParam> params)
      : Statement(AstNodeType::FunctionStatement), Identifier(identifier), Body(body), ReturnType(returnType),
        Params(params) {};

  void *accept(AstVisitor *visitor) override;
};

class ReturnStatement : public Statement
{
public:
  Token Lexeme;
  std::optional<Expression *> Value;

  ReturnStatement(Token lexeme) : Statement(AstNodeType::ReturnStatement), Lexeme(lexeme), Value(std::nullopt) {};
  ReturnStatement(Token lexeme, Expression *value)
      : Statement(AstNodeType::ReturnStatement), Lexeme(lexeme), Value(value) {};

  void *accept(AstVisitor *visitor) override;
};

class AST
{
public:
  std::vector<AstNode *> Nodes;
  const std::filesystem::path SourceFilePath;

  AST(std::filesystem::path sourcePath) : Nodes(), SourceFilePath(sourcePath) {};

  std::string ToStrng();
};
