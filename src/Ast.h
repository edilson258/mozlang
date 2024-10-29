#pragma once

#include "AstVisitor.h"
#include "Token.h"

#include <any>
#include <memory>
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

    virtual std::any accept(AstVisitor *) = 0;

  protected:
    AstNode(AstNodeType type) : Type(type){};
};

class Statement : public AstNode
{
  protected:
    Statement(AstNodeType type) : AstNode(type){};
};

class BlockStatement : public Statement
{
  public:
    std::vector<std::unique_ptr<Statement>> Statements;

    BlockStatement(std::vector<std::unique_ptr<Statement>> statements)
        : Statement(AstNodeType::BlockStatement), Statements(std::move(statements)){};

    std::any accept(AstVisitor *visitor) override;
};

class FunctionStatement : public Statement
{
  public:
    Token Identifier;
    BlockStatement Body;

    FunctionStatement(Token identifier, BlockStatement body)
        : Statement(AstNodeType::FunctionStatement), Identifier(identifier), Body(std::move(body)){};

    std::string GetName() const;
    std::any accept(AstVisitor *visitor) override;
};

class Expression : public Statement
{
  protected:
    Expression(AstNodeType type) : Statement(type){};
};

class IdentifierExpression : public Expression
{
  public:
    Token Identifier;

    IdentifierExpression(Token identifier) : Expression(AstNodeType::IdentifierExpression), Identifier(identifier){};

    std::string GetValue() const;
    std::any accept(AstVisitor *visitor) override;
};

class StringExpression : public Expression
{
  public:
    Token String;

    StringExpression(Token string) : Expression(AstNodeType::StringExpression), String(string){};

    std::string GetValue() const;
    std::any accept(AstVisitor *visitor) override;
};

class CallExpression : public Expression
{
  public:
    std::unique_ptr<Expression> Callee;
    std::vector<std::unique_ptr<Expression>> Args;

    CallExpression(std::unique_ptr<Expression> callee, std::vector<std::unique_ptr<Expression>> args)
        : Expression(AstNodeType::CallExpression), Callee(std::move(callee)), Args(std::move(args)){};

    std::any accept(AstVisitor *visitor) override;
};

class AST
{
  public:
    std::vector<std::unique_ptr<AstNode>> Nodes;

    AST() : Nodes(){};

    std::string ToStrng();
};