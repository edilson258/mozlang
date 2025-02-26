#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "token.h"
#include "type.h"

enum class Precedence
{
  LOWEST = 0,
  CALL = 10,
};

enum class StatementType
{
  BLOCK,
  FUNCTION,
  RETURN,
  EXPRESSION,
};

enum class ExpressionType
{
  CALL = 1,
  IDENTIFIER,
  STRING,
};

class Statement
{
public:
  Position Pos;
  StatementType Type;

protected:
  Statement(Position position, StatementType type) : Pos(position), Type(type) {};
};

class StatementBlock : public Statement
{
public:
  std::vector<std::shared_ptr<Statement>> Stmts;

  StatementBlock(Position position, std::vector<std::shared_ptr<Statement>> stmts) : Statement(position, StatementType::BLOCK), Stmts(std::move(stmts)) {};
};

class StatementReturn : public Statement
{
public:
  std::optional<std::shared_ptr<class Expression>> Value;
  StatementReturn(Position position, std::optional<std::shared_ptr<class Expression>> value) : Statement(position, StatementType::RETURN), Value(value) {};
};

class FunctionParam
{
public:
  Position Pos;
  std::shared_ptr<class ExpressionIdentifier> Name;
  std::optional<Type> Typ;

  FunctionParam(Position pos, std::shared_ptr<class ExpressionIdentifier> name, std::optional<Type> typ) : Pos(pos), Name(name), Typ(typ) {};
};

class FunctionParams
{
public:
  Position Pos;
  std::vector<FunctionParam> Params;

  FunctionParams(Position pos, std::vector<FunctionParam> params) : Pos(pos), Params(std::move(params)) {};
};

class StatementFunction : public Statement
{
public:
  std::shared_ptr<class ExpressionIdentifier> Name;
  FunctionParams Params;
  std::shared_ptr<StatementBlock> Body;

  StatementFunction(Position position, std::shared_ptr<class ExpressionIdentifier> name, FunctionParams params, std::shared_ptr<StatementBlock> body) : Statement(position, StatementType::FUNCTION), Name(name), Params(params), Body(body) {};
};

class Expression : public Statement
{
public:
  ExpressionType Type;

protected:
  Expression(Position position, ExpressionType type) : Statement(position, StatementType::EXPRESSION), Type(type) {};
};

class ExpressionCall : public Expression
{
public:
  std::shared_ptr<Expression> Callee;
  std::vector<std::shared_ptr<Expression>> Arguments;
  Position ArgumentsPosition;

  ExpressionCall(Position pos, std::shared_ptr<Expression> callee, std::vector<std::shared_ptr<Expression>> args, Position argsPos) : Expression(pos, ExpressionType::CALL), Callee(callee), Arguments(args), ArgumentsPosition(argsPos) {};
};

class ExpressionIdentifier : public Expression
{
public:
  std::string Value;

  ExpressionIdentifier(Position pos, std::string value) : Expression(pos, ExpressionType::IDENTIFIER), Value(value) {};
};

class ExpressionString : public Expression
{
public:
  std::string Value;

  ExpressionString(Position pos, std::string value) : Expression(pos, ExpressionType::STRING), Value(value) {};
};

class AST
{
public:
  std::vector<std::shared_ptr<Statement>> Program;
  AST() : Program() {};
  std::string Inspect();
};
