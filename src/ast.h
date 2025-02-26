#pragma once

#include <memory>
#include <string>
#include <vector>

#include "token.h"

enum class Precedence
{
  LOWEST = 0,
  CALL = 10,
};

enum class StatementType
{
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
