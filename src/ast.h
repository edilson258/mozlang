#pragma once

#include <cmath>
#include <optional>
#include <string>
#include <vector>

#include "token.h"
#include "type.h"

enum class Prec
{
  Low = 1,
  Assign = 2,
  Call = 10,
  FieldAcc = 11,
};

enum class StmtT
{
  Block = 1,
  Fun,
  Ret,
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
  CallExprArgs(Position position, std::vector<Expr *> args) : m_Position(position), m_Arguments(std::move(args)) {};

  Position GetPosition() const { return m_Position; }
  std::vector<Expr *> GetArguments() const { return m_Arguments; }

private:
  Position m_Position;
  std::vector<Expr *> m_Arguments;
};

class CallExpr : public Expr
{
public:
  CallExpr(Expr *callee, CallExprArgs args) : Expr(ExprT::Call), m_Callee(callee), m_Arguments(args) {};

  Position GetPosition() const override { return m_Callee->GetPosition().MergeWith(m_Arguments.GetPosition()); }
  Position GetCalleePosition() const { return m_Callee->GetPosition(); }
  Position GetArgumentsPosition() const { return m_Arguments.GetPosition(); }
  Expr *GetCallee() const { return m_Callee; }
  std::vector<Expr *> GetArguments() const { return m_Arguments.GetArguments(); }

private:
  Expr *m_Callee;
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
  AssignExpr(IdentExpr *dest, Expr *value) : Expr(ExprT::Assign), m_Dest(dest), m_Value(value) {};

  Position GetPosition() const override { return m_Dest->GetPosition().MergeWith(m_Value->GetPosition()); }
  IdentExpr *GetAssignee() const { return m_Dest; }
  Expr *GetValue() const { return m_Value; }

private:
  IdentExpr *m_Dest;
  Expr *m_Value;
};

class FieldAccExpr : public Expr
{
public:
  FieldAccExpr(Expr *value, IdentExpr *fieldName) : Expr(ExprT::FieldAcc), m_Value(value), m_FieldName(fieldName) {};

  Position GetPosition() const override { return m_Value->GetPosition().MergeWith(m_FieldName->GetPosition()); }
  Expr *GetValue() const { return m_Value; }
  IdentExpr *GetFieldName() const { return m_FieldName; }

private:
  Expr *m_Value;
  IdentExpr *m_FieldName;
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

enum class NumberBase
{
  Binary,
  Decimal,
  Hexadecimal,
};

class NumberExpr : public Expr
{
public:
private:
};

class BlockStmt : public Stmt
{
public:
  BlockStmt(Position position, std::vector<Stmt *> stmts) : Stmt(StmtT::Block), m_Position(position), m_Stmts(std::move(stmts)) {};

  std::vector<Stmt *> GetStatements() const { return m_Stmts; }
  Position GetPosition() const override { return m_Position; }

private:
  Position m_Position;
  std::vector<Stmt *> m_Stmts;
};

class RetStmt : public Stmt
{
public:
  RetStmt(Expr *value) : Stmt(StmtT::Ret), m_Pos(value->GetPosition()), m_Value(value) {};
  RetStmt(Position pos, Expr *value) : Stmt(StmtT::Ret), m_Pos(pos), m_Value(value) {};

  Expr *GetValue() const { return m_Value; }
  bool IsImplicity() const { return m_IsImplicity; }
  Position GetPosition() const override { return m_Pos.MergeWith(m_Value->GetPosition()); }

private:
  Position m_Pos;
  Expr *m_Value;
  bool m_IsImplicity;
};

class AstType
{
public:
  AstType(Position position, type::Type *type) : m_Position(position), m_Type(type) {};

  Position GetPosition() const { return m_Position; }
  type::Type *GetType() const { return m_Type; }

private:
  Position m_Position;
  type::Type *m_Type;
};

class FunParam
{
public:
  FunParam(IdentExpr *ident, AstType *astType) : m_Ident(ident), m_AstType(astType) {};

  std::string GetName() const { return m_Ident->GetValue(); }
  AstType *GetAstType() const { return m_AstType; }
  Position GetNamePosition() const { return m_Ident->GetPosition(); }
  Position GetPosition() const { return m_Ident->GetPosition().MergeWith(m_AstType->GetPosition()); }

private:
  IdentExpr *m_Ident;
  AstType *m_AstType;
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

class FunSign
{
public:
  FunSign(bool isPub, Position pos, IdentExpr *ident, FunParams params, AstType *retType) : m_IsPub(isPub), m_Pos(pos), m_Ident(ident), m_Params(params), m_RetType(retType) {};

  Position GetPosition() const { return m_Pos; }
  Position GetNamePosition() const { return m_Ident->GetPosition(); }
  Position GetParamsPosition() const { return m_Params.GetPosition(); }
  bool IsPub() const { return m_IsPub; }
  bool IsVarArgs() const { return m_Params.IsVarArgs(); }
  std::string GetName() const { return m_Ident->GetValue(); }
  std::vector<FunParam> GetParams() const { return m_Params.GetParams(); }
  AstType *GetRetType() const { return m_RetType; }

private:
  bool m_IsPub;
  Position m_Pos;
  IdentExpr *m_Ident;
  FunParams m_Params;
  AstType *m_RetType;
};

class FunStmt : public Stmt
{
public:
  FunStmt(FunSign sign, BlockStmt *body) : Stmt(StmtT::Fun), m_Sign(sign), m_Body(body) {};

  Position GetPosition() const override { return m_Sign.GetPosition().MergeWith(m_Body->GetPosition()); }
  BlockStmt *GetBody() const { return m_Body; }
  FunSign GetSign() const { return m_Sign; }

private:
  FunSign m_Sign;
  BlockStmt *m_Body;
};

class LetStmt : public Stmt
{
public:
  LetStmt(bool isPub, Position pos, IdentExpr *ident, AstType *astType, Expr *init) : Stmt(StmtT::Let), m_IsPub(isPub), m_Pos(pos), m_Ident(ident), m_AstType(astType), m_Init(init) {};

  bool IsPub() const { return m_IsPub; }
  Position GetPosition() const override { return m_Pos; }
  Position GetNamePosition() const { return m_Ident->GetPosition(); }
  std::string GetName() const { return m_Ident->GetValue(); }
  AstType *GetAstType() const { return m_AstType; }
  Expr *GetInit() const { return m_Init; }

private:
  bool m_IsPub;
  Position m_Pos;
  IdentExpr *m_Ident;
  AstType *m_AstType;
  Expr *m_Init;
};

class ImportStmt : public Stmt
{
public:
  ImportStmt(Token importToken, IdentExpr *alias, Token fromToken, std::optional<Token> atToken, std::vector<IdentExpr *> path) : Stmt(StmtT::Import), m_ImportToken(importToken), m_Name(alias), m_FromToken(fromToken), m_AtToken(atToken), m_Path(std::move(path)) {};

  Position GetPosition() const override { return m_ImportToken.m_Position.MergeWith(m_Path.back()->GetPosition()); }
  Position GetNamePosition() const { return m_Name->GetPosition(); }
  Position GetPathPosition() const { return m_Path.front()->GetPosition().MergeWith(m_Path.back()->GetPosition()); }
  std::string GetName() const { return m_Name->GetValue(); }
  std::vector<IdentExpr *> GetPath() const { return m_Path; }
  bool hasAtNotation() const { return m_AtToken.has_value(); }

private:
  Token m_ImportToken;
  IdentExpr *m_Name;
  Token m_FromToken;
  std::optional<Token> m_AtToken;
  std::vector<IdentExpr *> m_Path;
};

class AST
{
public:
  std::vector<Stmt *> m_Program;

  AST() : m_Program() {};

  std::string Inspect();
};
