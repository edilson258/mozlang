#pragma once

#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <vector>

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
  FUNCTION_SIGNATURE,
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
  StatementType GetType() const { return m_Type; }
  virtual Position GetPosition() const = 0;

  virtual ~Statement() = default;

protected:
  Statement(StatementType type) : m_Type(type) {};

private:
  StatementType m_Type;
};

class Expression : public Statement
{
public:
  ExpressionType GetType() const { return m_Type; }
  virtual Position GetPosition() const override = 0;

  virtual ~Expression() = default;

protected:
  Expression(ExpressionType type) : Statement(StatementType::EXPRESSION), m_Type(type) {};

private:
  ExpressionType m_Type;
};

class ExpressionCallArguments
{
public:
  ExpressionCallArguments(Position position, std::vector<std::shared_ptr<Expression>> args) : m_Position(position), m_Arguments(std::move(args)) {};

  Position GetPosition() const { return m_Position; }
  std::vector<std::shared_ptr<Expression>> GetArguments() const { return m_Arguments; }

private:
  Position m_Position;
  std::vector<std::shared_ptr<Expression>> m_Arguments;
};

class ExpressionCall : public Expression
{
public:
  ExpressionCall(std::shared_ptr<Expression> callee, ExpressionCallArguments args) : Expression(ExpressionType::CALL), m_Callee(callee), m_Arguments(args) {};

  Position GetPosition() const override { return m_Callee.get()->GetPosition().MergeWith(m_Arguments.GetPosition()); }
  Position GetCalleePosition() const { return m_Callee.get()->GetPosition(); }
  Position GetArgumentsPosition() const { return m_Arguments.GetPosition(); }
  std::shared_ptr<Expression> GetCallee() const { return m_Callee; }
  std::vector<std::shared_ptr<Expression>> GetArguments() const { return m_Arguments.GetArguments(); }

private:
  std::shared_ptr<Expression> m_Callee;
  ExpressionCallArguments m_Arguments;
};

class ExpressionIdentifier : public Expression
{
public:
  ExpressionIdentifier(Token token) : Expression(ExpressionType::IDENTIFIER), m_Token(token) {};

  std::string GetValue() const { return m_Token.m_Lexeme; }
  Position GetPosition() const override { return m_Token.m_Position; }

private:
  Token m_Token;
};

class ExpressionAssign : public Expression
{
public:
  ExpressionAssign(std::shared_ptr<ExpressionIdentifier> assignee, std::shared_ptr<Expression> value) : Expression(ExpressionType::ASSIGN), m_Assignee(assignee), m_Value(value) {};

  Position GetPosition() const override { return m_Assignee->GetPosition().MergeWith(m_Value->GetPosition()); }
  std::shared_ptr<ExpressionIdentifier> GetAssignee() const { return m_Assignee; }
  std::shared_ptr<Expression> GetValue() const { return m_Value; }

private:
  std::shared_ptr<ExpressionIdentifier> m_Assignee;
  std::shared_ptr<Expression> m_Value;
};

class ExpressionFieldAccess : public Expression
{
public:
  ExpressionFieldAccess(std::shared_ptr<Expression> value, std::shared_ptr<ExpressionIdentifier> fieldName) : Expression(ExpressionType::FIELD_ACCESS), m_Value(value), m_FieldName(fieldName) {};

  Position GetPosition() const override { return m_Value->GetPosition().MergeWith(m_FieldName->GetPosition()); }
  std::shared_ptr<Expression> GetValue() const { return m_Value; }
  std::shared_ptr<ExpressionIdentifier> GetFieldName() const { return m_FieldName; }

private:
  std::shared_ptr<Expression> m_Value;
  std::shared_ptr<ExpressionIdentifier> m_FieldName;
};

class ExpressionString : public Expression
{
public:
  ExpressionString(Token token) : Expression(ExpressionType::STRING), m_Token(token) {};

  std::string GetValue() const { return m_Token.m_Lexeme; }
  Position GetPosition() const override { return m_Token.m_Position; }

private:
  Token m_Token;
};

class StatementBlock : public Statement
{
public:
  StatementBlock(Position position, std::vector<std::shared_ptr<Statement>> stmts) : Statement(StatementType::BLOCK), m_Position(position), m_Stmts(std::move(stmts)) {};

  std::vector<std::shared_ptr<Statement>> GetStatements() const { return m_Stmts; }
  Position GetPosition() const override { return m_Position; }

private:
  Position m_Position;
  std::vector<std::shared_ptr<Statement>> m_Stmts;
};

class StatementReturn : public Statement
{
public:
  StatementReturn(std::shared_ptr<Expression> value) : Statement(StatementType::RETURN), m_ReturnToken(std::nullopt), m_Value(value) {};
  StatementReturn(Token returnToken, std::optional<std::shared_ptr<Expression>> value) : Statement(StatementType::RETURN), m_ReturnToken(returnToken), m_Value(value) {};

  std::optional<std::shared_ptr<Expression>> GetValue() const { return m_Value; }
  bool IsImplicity() const { return !m_ReturnToken.has_value(); }
  Position GetPosition() const override
  {
    if (m_ReturnToken.has_value() && m_Value.has_value())
    {
      return m_ReturnToken.value().m_Position.MergeWith(m_Value.value().get()->GetPosition());
    }
    if (m_ReturnToken.has_value())
    {
      return m_ReturnToken.value().m_Position;
    }
    return m_Value.value().get()->GetPosition();
  }

private:
  std::optional<Token> m_ReturnToken;
  std::optional<std::shared_ptr<Expression>> m_Value;
};

class AstType
{
public:
  AstType(Position position, std::shared_ptr<type::Type> type) : m_Position(position), m_Type(type) {};

  Position GetPosition() const { return m_Position; }
  std::shared_ptr<type::Type> GetType() const { return m_Type; }

private:
  Position m_Position;
  std::shared_ptr<type::Type> m_Type;
};

class FunctionParam
{
public:
  FunctionParam(std::shared_ptr<ExpressionIdentifier> identifier, std::optional<AstType> astTypeAnnotation) : m_Identifier(identifier), m_AstTypeOpt(astTypeAnnotation) {};

  std::string GetName() const { return m_Identifier.get()->GetValue(); }
  std::optional<AstType> GetAstType() const { return m_AstTypeOpt; }
  Position GetNamePosition() const { return m_Identifier.get()->GetPosition(); }
  Position GetPosition() const { return m_AstTypeOpt.has_value() ? m_Identifier.get()->GetPosition().MergeWith(m_AstTypeOpt.value().GetPosition()) : m_Identifier.get()->GetPosition(); }

private:
  std::shared_ptr<ExpressionIdentifier> m_Identifier;
  std::optional<AstType> m_AstTypeOpt;
};

class FunctionParams
{
public:
  FunctionParams(Position position, std::vector<FunctionParam> params) : m_Position(position), m_Params(std::move(params)) {};

  Position GetPosition() const { return m_Position; }
  std::vector<FunctionParam> GetParams() const { return m_Params; }

private:
  Position m_Position;
  std::vector<FunctionParam> m_Params;
};

class PubAccessModifier
{
public:
  PubAccessModifier(Token token) : m_Token(token) {};

  Position GetPosition() const { return m_Token.m_Position; }

private:
  Token m_Token;
};

class FunDeclarator
{
public:
  FunDeclarator(Token token) : m_Token(token) {};

  Position GetPosition() const { return m_Token.m_Position; }

private:
  Token m_Token;
};

class StatementFunctionSignature : public Statement
{
public:
  StatementFunctionSignature(std::optional<PubAccessModifier> pubAccessModifier, FunDeclarator declarator, std::shared_ptr<ExpressionIdentifier> identifier, FunctionParams params, std::optional<AstType> returnType) : Statement(StatementType::FUNCTION_SIGNATURE), m_AccessModifier(pubAccessModifier), m_Declarator(declarator), m_Identifier(identifier), m_Params(params), m_ReturnType(returnType) {};

  Position GetPosition() const override
  {
    Position position = m_AccessModifier.has_value() ? m_AccessModifier.value().GetPosition() : m_Declarator.GetPosition();
    return m_ReturnType.has_value() ? position.MergeWith(m_ReturnType.value().GetPosition()) : position.MergeWith(m_Params.GetPosition());
  }

  std::optional<PubAccessModifier> m_AccessModifier;
  FunDeclarator m_Declarator;
  std::shared_ptr<ExpressionIdentifier> m_Identifier;
  FunctionParams m_Params;
  std::optional<AstType> m_ReturnType;
};

class StatementFunction : public Statement
{
public:
  StatementFunction(std::shared_ptr<StatementFunctionSignature> signature, std::shared_ptr<StatementBlock> body) : Statement(StatementType::FUNCTION), m_Signature(signature), m_Body(body) {};

  bool IsPub() const { return m_Signature.get()->m_AccessModifier.has_value(); }
  Position GetPosition() const override { return m_Signature.get()->GetPosition().MergeWith(m_Body.get()->GetPosition()); }
  Position GetNamePosition() const { return m_Signature.get()->m_Identifier.get()->GetPosition(); }
  Position GetParamsPosition() const { return m_Signature.get()->m_Params.GetPosition(); }
  std::string GetName() const { return m_Signature.get()->m_Identifier.get()->GetValue(); }
  std::vector<FunctionParam> GetParams() const { return m_Signature.get()->m_Params.GetParams(); }
  std::optional<AstType> GetReturnType() const { return m_Signature.get()->m_ReturnType; }
  std::shared_ptr<StatementBlock> GetBody() const { return m_Body; }

private:
  std::shared_ptr<StatementFunctionSignature> m_Signature;
  std::shared_ptr<StatementBlock> m_Body;
};

class LetDeclarator
{
public:
  LetDeclarator(Token token) : m_Token(token) {};

  Position GetPosition() const { return m_Token.m_Position; }

private:
  Token m_Token;
};

class StatementLet : public Statement
{
public:
  StatementLet(std::optional<PubAccessModifier> pubAccessModifier, LetDeclarator declarator, std::shared_ptr<ExpressionIdentifier> identifier, std::optional<AstType> typeAnnotation, std::optional<std::shared_ptr<Expression>> initializer) : Statement(StatementType::LET), m_PubAccessModifier(pubAccessModifier), m_Declarator(declarator), m_Identifier(identifier), m_AstType(typeAnnotation), m_Initializer(initializer) {};

  bool IsPub() const { return m_PubAccessModifier.has_value(); }
  Position GetPosition() const override
  {
    Position position = m_PubAccessModifier.has_value() ? m_PubAccessModifier.value().GetPosition() : m_Declarator.GetPosition();
    return m_Initializer.has_value() ? position.MergeWith(m_Initializer.value().get()->GetPosition()) : m_AstType.has_value() ? position.MergeWith(m_AstType.value().GetPosition())
                                                                                                                              : position.MergeWith(m_Identifier.get()->GetPosition());
  }
  Position GetNamePosition() const { return m_Identifier->GetPosition(); }
  std::string GetName() const { return m_Identifier->GetValue(); }
  std::optional<AstType> GetAstType() const { return m_AstType; }
  std::optional<std::shared_ptr<Expression>> GetInitializer() const { return m_Initializer; }

private:
  std::optional<PubAccessModifier> m_PubAccessModifier;
  LetDeclarator m_Declarator;
  std::shared_ptr<ExpressionIdentifier> m_Identifier;
  std::optional<AstType> m_AstType;
  std::optional<std::shared_ptr<Expression>> m_Initializer;
};

class StatementImport : public Statement
{
public:
  StatementImport(Token importToken, std::shared_ptr<ExpressionIdentifier> alias, Token fromToken, std::shared_ptr<ExpressionString> path) : Statement(StatementType::IMPORT), m_ImportToken(importToken), m_Alias(alias), m_FromToken(fromToken), m_Path(path) {};

  Position GetPosition() const override { return m_ImportToken.m_Position.MergeWith(m_Path.get()->GetPosition()); }
  Position GetNamePosition() const { return m_Alias.get()->GetPosition(); }
  Position GetPathPosition() const { return m_Path.get()->GetPosition(); }
  std::string GetName() const { return m_Alias.get()->GetValue(); }
  std::string GetPath() const { return m_Path.get()->GetValue(); }

private:
  Token m_ImportToken;
  std::shared_ptr<ExpressionIdentifier> m_Alias;
  Token m_FromToken;
  std::shared_ptr<ExpressionString> m_Path;
};

class AST
{
public:
  std::vector<std::shared_ptr<Statement>> m_Program;

  AST() : m_Program() {};

  std::string Inspect();
};
