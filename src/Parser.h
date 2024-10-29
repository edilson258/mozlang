#pragma once

#include "Lexer.h"
#include "Token.h"

#include <any>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

enum class AstNodeType
{
    BlockStatement,
    FunctionStatement,
    CallExpression,
    IdentifierExpression,
    StringExpression,
};

class BlockStatement;
class FunctionStatement;
class CallExpression;
class IdentifierExpression;
class StringExpression;

class AstVisitor
{
  public:
    virtual std::any visit(BlockStatement *) = 0;
    virtual std::any visit(FunctionStatement *) = 0;
    virtual std::any visit(CallExpression *) = 0;
    virtual std::any visit(IdentifierExpression *) = 0;
    virtual std::any visit(StringExpression *) = 0;
};

class AstInspector : public AstVisitor
{
  public:
    std::any visit(BlockStatement *) override;
    std::any visit(FunctionStatement *) override;
    std::any visit(CallExpression *) override;
    std::any visit(IdentifierExpression *) override;
    std::any visit(StringExpression *) override;

    AstInspector() : TabRate(4), TabSize(0){};

    std::string GetValue() const { return Out.str(); }

  private:
    unsigned long TabSize;
    unsigned long TabRate;

    std::ostringstream Out;

    void Write(std::string);

    void Tab() { TabSize += TabRate; }
    void UnTab() { TabSize -= TabRate; };
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

    std::any accept(AstVisitor *visitor) override { return visitor->visit(this); }
};

class FunctionStatement : public Statement
{
  public:
    Token Identifier;
    BlockStatement Body;

    FunctionStatement(Token identifier, BlockStatement body)
        : Statement(AstNodeType::FunctionStatement), Identifier(identifier), Body(std::move(body)){};

    std::string GetName() const { return std::get<std::string>(Identifier.Data); }

    std::any accept(AstVisitor *visitor) override { return visitor->visit(this); }
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

    std::string GetValue() const { return std::get<std::string>(Identifier.Data); }

    std::any accept(AstVisitor *visitor) override { return visitor->visit(this); }
};

class StringExpression : public Expression
{
  public:
    Token String;

    StringExpression(Token string) : Expression(AstNodeType::StringExpression), String(string){};

    std::string GetValue() const { return std::get<std::string>(String.Data); }

    std::any accept(AstVisitor *visitor) override { return visitor->visit(this); }
};

class CallExpression : public Expression
{
  public:
    std::unique_ptr<Expression> Callee;
    std::vector<std::unique_ptr<Expression>> Args;

    CallExpression(std::unique_ptr<Expression> callee, std::vector<std::unique_ptr<Expression>> args)
        : Expression(AstNodeType::CallExpression), Callee(std::move(callee)), Args(std::move(args)){};

    std::any accept(AstVisitor *visitor) override { return visitor->visit(this); }
};

class AST
{
  public:
    std::vector<std::unique_ptr<AstNode>> Nodes;

    AST() : Nodes(){};

    std::string ToStrng();
};

enum class Precedence
{
    Lowest = 1,
    Call = 2,
};

class Parser
{
  public:
    Lexer &lexer;

    AST Parse();

    Parser(Lexer &lex) : lexer(lex){};

  private:
    Token CurrentToken;
    Token NextToken;

    void Bump();
    void BumpExpected(TokenType);

    Precedence TokenToPrecedence(Token &);
    Precedence GetCurrentTokenPrecedence();

    // Parsers
    BlockStatement ParseBlockStatement();
    std::unique_ptr<Statement> ParseStatement();
    std::unique_ptr<FunctionStatement> ParseFunctionStatement();
    std::unique_ptr<Expression> ParseExpressionStatement(Precedence);
    std::unique_ptr<CallExpression> ParseCallExpression(std::unique_ptr<Expression>);
};