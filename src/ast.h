#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "loader.h"
#include "token.h"
#include "type.h"

enum class Precedence
{
  LOWEST = 1,
  ASSIGN = 2,
  CALL = 10,
  FIELD_ACCESS = 11,
};

enum class StatementType
{
  BLOCK = 1,
  FUNCTION,
  RETURN,
  EXPRESSION,
  LET,
  IMPORT,
};

enum class ExpressionType
{
  CALL = 1,
  IDENTIFIER,
  STRING,
  ASSIGN,
  FIELD_ACCESS,
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
  std::shared_ptr<type::Type> m_Type;

  TypeAnnotationToken(std::optional<Position> position, std::shared_ptr<type::Type> returnType) : m_Position(position), m_Type(returnType) {};
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

class StatementLet : public Statement
{
public:
  std::shared_ptr<class ExpressionIdentifier> m_Identifier;
  TypeAnnotationToken m_TypeAnnotation;
  std::optional<std::shared_ptr<class Expression>> m_Initializer;

  StatementLet(Position position, std::shared_ptr<class ExpressionIdentifier> identifier, TypeAnnotationToken typeAnnotation, std::optional<std::shared_ptr<class Expression>> initializer) : Statement(position, StatementType::LET), m_Identifier(identifier), m_TypeAnnotation(typeAnnotation), m_Initializer(initializer) {};
};

class StatementImport : public Statement
{
public:
  std::shared_ptr<class ExpressionIdentifier> m_Alias;
  std::string m_Path;

  StatementImport(Position position, std::shared_ptr<class ExpressionIdentifier> alias, std::string path) : Statement(position, StatementType::IMPORT), m_Alias(alias), m_Path(path) {};
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

class ExpressionAssign : public Expression
{
public:
  std::shared_ptr<ExpressionIdentifier> m_Assignee;
  std::shared_ptr<Expression> m_Value;

  ExpressionAssign(Position position, std::shared_ptr<ExpressionIdentifier> assignee, std::shared_ptr<Expression> value) : Expression(position, ExpressionType::ASSIGN), m_Assignee(assignee), m_Value(value) {};
};

class ExpressionFieldAccess : public Expression
{
public:
  std::shared_ptr<Expression> m_Value;
  std::shared_ptr<ExpressionIdentifier> m_FieldName;

  ExpressionFieldAccess(Position position, std::shared_ptr<Expression> value, std::shared_ptr<ExpressionIdentifier> fieldName) : Expression(position, ExpressionType::FIELD_ACCESS), m_Value(value), m_FieldName(fieldName) {};
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
  ModuleID m_ModuleID;
  std::vector<std::shared_ptr<Statement>> m_Program;

  AST(ModuleID moduleID) : m_ModuleID(moduleID), m_Program() {};

  std::string Inspect();
};
