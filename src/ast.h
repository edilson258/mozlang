#pragma once

#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "token.h"
#include "type.h"

enum class Prec
{
  LOW = 1,
  ASSIGN = 2,
  CALL = 10,
  FIELD_ACC = 11,
};

enum class StmtT
{
  Block = 1,
  Fun,
  FunSign,
  Return,
  Expr,
  Let,
  Import,
};

enum class ExprT
{
  Call = 1,
  Ident,
  String,
  Assign,
  FieldAcc,
};

class Stmt
{
public:
  StmtT GetType() const { return m_Type; }
  virtual Position GetPosition() const = 0;

  virtual ~Stmt() = default;

protected:
  Stmt(StmtT type) : m_Type(type) {};

private:
  StmtT m_Type;
};

class Expr : public Stmt
{
public:
  ExprT GetType() const { return m_Type; }
  virtual Position GetPosition() const override = 0;

  virtual ~Expr() = default;

protected:
  Expr(ExprT type) : Stmt(StmtT::Expr), m_Type(type) {};

private:
  ExprT m_Type;
};

class CallExprArgs
{
public:
  CallExprArgs(Position position, std::vector<std::shared_ptr<Expr>> args) : m_Position(position), m_Arguments(std::move(args)) {};

  Position GetPosition() const { return m_Position; }
  std::vector<std::shared_ptr<Expr>> GetArguments() const { return m_Arguments; }

private:
  Position m_Position;
  std::vector<std::shared_ptr<Expr>> m_Arguments;
};

class CallExpr : public Expr
{
public:
  CallExpr(std::shared_ptr<Expr> callee, CallExprArgs args) : Expr(ExprT::Call), m_Callee(callee), m_Arguments(args) {};

  Position GetPosition() const override { return m_Callee.get()->GetPosition().MergeWith(m_Arguments.GetPosition()); }
  Position GetCalleePosition() const { return m_Callee.get()->GetPosition(); }
  Position GetArgumentsPosition() const { return m_Arguments.GetPosition(); }
  std::shared_ptr<Expr> GetCallee() const { return m_Callee; }
  std::vector<std::shared_ptr<Expr>> GetArguments() const { return m_Arguments.GetArguments(); }

private:
  std::shared_ptr<Expr> m_Callee;
  CallExprArgs m_Arguments;
};

class IdentExpr : public Expr
{
public:
  IdentExpr(Token token) : Expr(ExprT::Ident), m_Token(token) {};

  std::string GetValue() const { return m_Token.m_Lexeme; }
  Position GetPosition() const override { return m_Token.m_Position; }

private:
  Token m_Token;
};

class AssignExpr : public Expr
{
public:
  AssignExpr(std::shared_ptr<IdentExpr> assignee, std::shared_ptr<Expr> value) : Expr(ExprT::Assign), m_Assignee(assignee), m_Value(value) {};

  Position GetPosition() const override { return m_Assignee->GetPosition().MergeWith(m_Value->GetPosition()); }
  std::shared_ptr<IdentExpr> GetAssignee() const { return m_Assignee; }
  std::shared_ptr<Expr> GetValue() const { return m_Value; }

private:
  std::shared_ptr<IdentExpr> m_Assignee;
  std::shared_ptr<Expr> m_Value;
};

class FieldAccExpr : public Expr
{
public:
  FieldAccExpr(std::shared_ptr<Expr> value, std::shared_ptr<IdentExpr> fieldName) : Expr(ExprT::FieldAcc), m_Value(value), m_FieldName(fieldName) {};

  Position GetPosition() const override { return m_Value->GetPosition().MergeWith(m_FieldName->GetPosition()); }
  std::shared_ptr<Expr> GetValue() const { return m_Value; }
  std::shared_ptr<IdentExpr> GetFieldName() const { return m_FieldName; }

private:
  std::shared_ptr<Expr> m_Value;
  std::shared_ptr<IdentExpr> m_FieldName;
};

class StringExpr : public Expr
{
public:
  StringExpr(Token token) : Expr(ExprT::String), m_Token(token) {};

  std::string GetValue() const { return m_Token.m_Lexeme; }
  Position GetPosition() const override { return m_Token.m_Position; }

private:
  Token m_Token;
};

class BlockStmt : public Stmt
{
public:
  BlockStmt(Position position, std::vector<std::shared_ptr<Stmt>> stmts) : Stmt(StmtT::Block), m_Position(position), m_Stmts(std::move(stmts)) {};

  std::vector<std::shared_ptr<Stmt>> GetStatements() const { return m_Stmts; }
  Position GetPosition() const override { return m_Position; }

private:
  Position m_Position;
  std::vector<std::shared_ptr<Stmt>> m_Stmts;
};

class RetStmt : public Stmt
{
public:
  RetStmt(std::shared_ptr<Expr> value) : Stmt(StmtT::Return), m_ReturnToken(std::nullopt), m_Value(value) {};
  RetStmt(Token returnToken, std::optional<std::shared_ptr<Expr>> value) : Stmt(StmtT::Return), m_ReturnToken(returnToken), m_Value(value) {};

  std::optional<std::shared_ptr<Expr>> GetValue() const { return m_Value; }
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
  std::optional<std::shared_ptr<Expr>> m_Value;
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

class FunParam
{
public:
  FunParam(std::shared_ptr<IdentExpr> identifier, std::optional<AstType> astTypeAnnotation) : m_Identifier(identifier), m_AstTypeOpt(astTypeAnnotation) {};

  std::string GetName() const { return m_Identifier.get()->GetValue(); }
  std::optional<AstType> GetAstType() const { return m_AstTypeOpt; }
  Position GetNamePosition() const { return m_Identifier.get()->GetPosition(); }
  Position GetPosition() const { return m_AstTypeOpt.has_value() ? m_Identifier.get()->GetPosition().MergeWith(m_AstTypeOpt.value().GetPosition()) : m_Identifier.get()->GetPosition(); }

private:
  std::shared_ptr<IdentExpr> m_Identifier;
  std::optional<AstType> m_AstTypeOpt;
};

class Ellipsis
{
public:
  Ellipsis(Token token) : m_Token(token) {};

private:
  Token m_Token;
};

class FunParams
{
public:
  FunParams(Position position, std::vector<FunParam> params, std::optional<Ellipsis> varArgsNotation) : m_Position(position), m_Params(std::move(params)), m_VarArgsNotation(varArgsNotation) {};

  Position GetPosition() const { return m_Position; }
  std::vector<FunParam> GetParams() const { return m_Params; }
  bool IsVarArgs() const { return m_VarArgsNotation.has_value(); }

private:
  Position m_Position;
  std::vector<FunParam> m_Params;
  std::optional<Ellipsis> m_VarArgsNotation;
};

class PubAccMod
{
public:
  PubAccMod(Token token) : m_Token(token) {};

  Position GetPosition() const { return m_Token.m_Position; }

private:
  Token m_Token;
};

class FunDecl
{
public:
  FunDecl(Token token) : m_Token(token) {};

  Position GetPosition() const { return m_Token.m_Position; }

private:
  Token m_Token;
};

class FunSignStmt : public Stmt
{
public:
  FunSignStmt(std::optional<PubAccMod> pubAccessModifier, FunDecl declarator, std::shared_ptr<IdentExpr> identifier, FunParams params, std::optional<AstType> returnType) : Stmt(StmtT::FunSign), m_AccessModifier(pubAccessModifier), m_Declarator(declarator), m_Identifier(identifier), m_Params(params), m_ReturnType(returnType) {};

  Position GetPosition() const override
  {
    Position position = m_AccessModifier.has_value() ? m_AccessModifier.value().GetPosition() : m_Declarator.GetPosition();
    return m_ReturnType.has_value() ? position.MergeWith(m_ReturnType.value().GetPosition()) : position.MergeWith(m_Params.GetPosition());
  }
  Position GetNamePosition() const { return m_Identifier.get()->GetPosition(); }
  Position GetParamsPosition() const { return m_Params.GetPosition(); }
  bool IsPub() const { return m_AccessModifier.has_value(); }
  bool IsVarArgs() const { return m_Params.IsVarArgs(); }
  std::string GetName() const { return m_Identifier.get()->GetValue(); }
  std::vector<FunParam> GetParams() const { return m_Params.GetParams(); }
  std::optional<AstType> GetReturnType() const { return m_ReturnType; }

private:
  std::optional<PubAccMod> m_AccessModifier;
  FunDecl m_Declarator;
  std::shared_ptr<IdentExpr> m_Identifier;
  FunParams m_Params;
  std::optional<AstType> m_ReturnType;
};

class FunStmt : public Stmt
{
public:
  FunStmt(std::shared_ptr<FunSignStmt> signature, std::shared_ptr<BlockStmt> body) : Stmt(StmtT::Fun), m_Signature(signature), m_Body(body) {};

  Position GetPosition() const override { return m_Signature.get()->GetPosition().MergeWith(m_Body.get()->GetPosition()); }
  std::shared_ptr<BlockStmt> GetBody() const { return m_Body; }
  std::shared_ptr<FunSignStmt> GetSignature() const { return m_Signature; }

private:
  std::shared_ptr<FunSignStmt> m_Signature;
  std::shared_ptr<BlockStmt> m_Body;
};

class LetDecl
{
public:
  LetDecl(Token token) : m_Token(token) {};

  Position GetPosition() const { return m_Token.m_Position; }

private:
  Token m_Token;
};

class LetStmt : public Stmt
{
public:
  LetStmt(std::optional<PubAccMod> pubAccessModifier, LetDecl declarator, std::shared_ptr<IdentExpr> identifier, std::optional<AstType> typeAnnotation, std::optional<std::shared_ptr<Expr>> initializer) : Stmt(StmtT::Let), m_PubAccessModifier(pubAccessModifier), m_Declarator(declarator), m_Identifier(identifier), m_AstType(typeAnnotation), m_Initializer(initializer) {};

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
  std::optional<std::shared_ptr<Expr>> GetInitializer() const { return m_Initializer; }

private:
  std::optional<PubAccMod> m_PubAccessModifier;
  LetDecl m_Declarator;
  std::shared_ptr<IdentExpr> m_Identifier;
  std::optional<AstType> m_AstType;
  std::optional<std::shared_ptr<Expr>> m_Initializer;
};

class ImportStmt : public Stmt
{
public:
  ImportStmt(Token importToken, std::shared_ptr<IdentExpr> alias, Token fromToken, std::optional<Token> atToken, std::vector<std::shared_ptr<IdentExpr>> path) : Stmt(StmtT::Import), m_ImportToken(importToken), m_Alias(alias), m_FromToken(fromToken), m_AtToken(atToken), m_Path(std::move(path)) {};

  Position GetPosition() const override { return m_ImportToken.m_Position.MergeWith(m_Path.back().get()->GetPosition()); }
  Position GetNamePosition() const { return m_Alias.get()->GetPosition(); }
  Position GetPathPosition() const { return m_Alias.get()->GetPosition(); }
  std::string GetName() const { return m_Alias.get()->GetValue(); }
  std::vector<std::shared_ptr<IdentExpr>> GetPath() const { return m_Path; }

private:
  Token m_ImportToken;
  std::shared_ptr<IdentExpr> m_Alias;
  Token m_FromToken;
  std::optional<Token> m_AtToken;
  std::vector<std::shared_ptr<IdentExpr>> m_Path;
};

class AST
{
public:
  std::vector<std::shared_ptr<Stmt>> m_Program;

  AST() : m_Program() {};

  std::string Inspect();
};
