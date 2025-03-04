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
  BLOCK = 1,
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
  Position m_Position;
  StatementType m_Type;

protected:
  Statement(Position position, StatementType type) : m_Position(position), m_Type(type) {};
};

class StatementBlock : public Statement
{
public:
  std::vector<std::shared_ptr<Statement>> m_Stmts;

  StatementBlock(Position position, std::vector<std::shared_ptr<Statement>> stmts) : Statement(position, StatementType::BLOCK), m_Stmts(std::move(stmts)) {};
};

enum class StatementReturnType
{
  EXPLICITY,
  IMPLICITY,
};

class StatementReturn : public Statement
{
public:
  std::optional<std::shared_ptr<class Expression>> m_Value;
  StatementReturnType m_Type;
  StatementReturn(Position position, std::optional<std::shared_ptr<class Expression>> value, StatementReturnType type = StatementReturnType::EXPLICITY) : Statement(position, StatementType::RETURN), m_Value(value), m_Type(type) {};
};

class TypeAnnotationToken
{
public:
  std::optional<Position> m_Position;
  std::shared_ptr<type::Type> m_ReturnType;

  TypeAnnotationToken(std::optional<Position> position, std::shared_ptr<type::Type> returnType) : m_Position(position), m_ReturnType(returnType) {};
};

class FunctionParam
{
public:
  Position m_Position;
  std::shared_ptr<class ExpressionIdentifier> m_Identifier;
  TypeAnnotationToken m_TypeAnnotation;

  FunctionParam(Position position, std::shared_ptr<class ExpressionIdentifier> identifier, TypeAnnotationToken typeAnnotation) : m_Position(position), m_Identifier(identifier), m_TypeAnnotation(typeAnnotation) {};
};

class FunctionParams
{
public:
  Position m_Position;
  std::vector<FunctionParam> m_Params;

  FunctionParams(Position position, std::vector<FunctionParam> params) : m_Position(position), m_Params(std::move(params)) {};
};

class StatementFunction : public Statement
{
public:
  FunctionParams m_Params;
  std::shared_ptr<StatementBlock> m_Body;
  std::shared_ptr<class ExpressionIdentifier> m_Identifier;
  TypeAnnotationToken m_ReturnTypeAnnotation;

  StatementFunction(Position position, std::shared_ptr<class ExpressionIdentifier> identifier, FunctionParams params, std::shared_ptr<StatementBlock> body, TypeAnnotationToken typeAnnotationToken) : Statement(position, StatementType::FUNCTION), m_Params(params), m_Body(body), m_Identifier(identifier), m_ReturnTypeAnnotation(typeAnnotationToken) {};
};

class Expression : public Statement
{
public:
  ExpressionType m_Type;

protected:
  Expression(Position position, ExpressionType type) : Statement(position, StatementType::EXPRESSION), m_Type(type) {};
};

class ExpressionCall : public Expression
{
public:
  std::shared_ptr<Expression> m_Callee;
  std::vector<std::shared_ptr<Expression>> m_Arguments;
  Position m_ArgumentsPosition;

  ExpressionCall(Position position, std::shared_ptr<Expression> callee, std::vector<std::shared_ptr<Expression>> args, Position argsPos) : Expression(position, ExpressionType::CALL), m_Callee(callee), m_Arguments(args), m_ArgumentsPosition(argsPos) {};
};

class ExpressionIdentifier : public Expression
{
public:
  std::string m_Value;

  ExpressionIdentifier(Position position, std::string value) : Expression(position, ExpressionType::IDENTIFIER), m_Value(value) {};
};

class ExpressionString : public Expression
{
public:
  std::string m_Value;

  ExpressionString(Position position, std::string value) : Expression(position, ExpressionType::STRING), m_Value(value) {};
};

class AST
{
public:
  std::vector<std::shared_ptr<Statement>> m_Program;
  AST() : m_Program() {};
  std::string Inspect();
};
