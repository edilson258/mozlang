#pragma once

#include <cmath>
#include <optional>
#include <string>
#include <vector>

#include "pointer.h"
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
  Number,
  Assign,
  FieldAcc,
};

class Stmt
{
public:
  StmtT GetType() const { return m_Type; }
  virtual Position GetPos() const = 0;

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
  virtual Position GetPos() const override = 0;

  virtual ~Expr() = default;

protected:
  Expr(ExprT type) : Stmt(StmtT::Expr), m_Type(type) {};

private:
  ExprT m_Type;
};

/*
  Call expression
*/
class CallExprArgs
{
public:
  CallExprArgs(Position position, std::vector<Ptr<Expr>> args) : m_Pos(position), m_Args(std::move(args)) {};

  Position GetPos() const { return m_Pos; }
  std::vector<Ptr<Expr>> GetArgs() const { return m_Args; }

private:
  Position m_Pos;
  std::vector<Ptr<Expr>> m_Args;
};

class CallExpr : public Expr
{
public:
  CallExpr(Ptr<Expr> callee, CallExprArgs args) : Expr(ExprT::Call), m_Callee(callee), m_Args(args) {};

  Position GetPos() const override { return m_Callee->GetPos().MergeWith(m_Args.GetPos()); }
  Position GetCalleePos() const { return m_Callee->GetPos(); }
  Position GetArgsPos() const { return m_Args.GetPos(); }
  Ptr<Expr> GetCallee() const { return m_Callee; }
  std::vector<Ptr<Expr>> GetArgs() const { return m_Args.GetArgs(); }

private:
  Ptr<Expr> m_Callee;
  CallExprArgs m_Args;
};

/*
  Ident expression
*/
class IdentExpr : public Expr
{
public:
  IdentExpr(Token token) : Expr(ExprT::Ident), m_Token(token) {};

  std::string GetValue() const { return m_Token.m_Lexeme; }
  Position GetPos() const override { return m_Token.m_Position; }

private:
  Token m_Token;
};

/*
  Assign expression
*/
class AssignExpr : public Expr
{
public:
  AssignExpr(Ptr<IdentExpr> dest, Ptr<Expr> value) : Expr(ExprT::Assign), m_Dest(dest), m_Value(value) {};

  Position GetPos() const override { return m_Dest->GetPos().MergeWith(m_Value->GetPos()); }
  Ptr<IdentExpr> GetDest() const { return m_Dest; }
  Ptr<Expr> GetValue() const { return m_Value; }

private:
  Ptr<IdentExpr> m_Dest;
  Ptr<Expr> m_Value;
};

/*
  Field Access expression
*/
class FieldAccExpr : public Expr
{
public:
  FieldAccExpr(Ptr<Expr> value, Ptr<IdentExpr> fieldName) : Expr(ExprT::FieldAcc), m_Value(value), m_FieldName(fieldName) {};

  Position GetPos() const override { return m_Value->GetPos().MergeWith(m_FieldName->GetPos()); }
  Ptr<Expr> GetValue() const { return m_Value; }
  Ptr<IdentExpr> GetFieldName() const { return m_FieldName; }

private:
  Ptr<Expr> m_Value;
  Ptr<IdentExpr> m_FieldName;
};

class StringExpr : public Expr
{
public:
  StringExpr(Token token) : Expr(ExprT::String), m_Token(token) {};

  std::string GetValue() const { return m_Token.m_Lexeme; }
  Position GetPos() const override { return m_Token.m_Position; }

private:
  Token m_Token;
};

enum class NumberBase
{
  Bin = 2,
  Dec = 10,
  Hex = 16,
};

class NumberExpr : public Expr
{
public:
  Position m_Pos;
  std::string m_Raw;
  NumberBase m_Base;

  bool m_IsFloat;
  NumberExpr(Position pos, std::string raw, NumberBase base, bool isFloat = false) : Expr(ExprT::Number), m_Pos(pos), m_Raw(raw), m_Base(base), m_IsFloat(isFloat) {};

  Position GetPos() const override { return m_Pos; }
  bool IsFloat() const { return m_IsFloat; }
  NumberBase GetBase() const { return m_Base; }
  std::string GetRaw() const { return m_Raw; }
};

class BlockStmt : public Stmt
{
public:
  BlockStmt(Position position, std::vector<Ptr<Stmt>> stmts) : Stmt(StmtT::Block), m_Position(position), m_Stmts(std::move(stmts)) {};

  std::vector<Ptr<Stmt>> GetStatements() const { return m_Stmts; }
  Position GetPos() const override { return m_Position; }

private:
  Position m_Position;
  std::vector<Ptr<Stmt>> m_Stmts;
};

class RetStmt : public Stmt
{
public:
  RetStmt(Ptr<Expr> value) : Stmt(StmtT::Ret), m_Pos(value->GetPos()), m_Value(value), m_IsImplicity(true) {};
  RetStmt(Position pos, Ptr<Expr> value) : Stmt(StmtT::Ret), m_Pos(pos), m_Value(value), m_IsImplicity(false) {};

  Ptr<Expr> GetValue() const { return m_Value; }
  bool IsExplicity() const { return !m_IsImplicity; }
  Position GetPos() const override { return m_Value ? m_Pos.MergeWith(m_Value->GetPos()) : m_Pos; }

private:
  Position m_Pos;
  Ptr<Expr> m_Value;
  bool m_IsImplicity;
};

class AstType
{
public:
  AstType(Position position, Ptr<type::Type> type) : m_Position(position), m_Type(type) {};

  Position GetPos() const { return m_Position; }
  Ptr<type::Type> GetType() const { return m_Type; }

private:
  Position m_Position;
  Ptr<type::Type> m_Type;
};

class FunParam
{
public:
  FunParam(Ptr<IdentExpr> ident, Ptr<AstType> astType) : m_Ident(ident), m_AstType(astType) {};

  std::string GetName() const { return m_Ident->GetValue(); }
  Ptr<AstType> GetAstType() const { return m_AstType; }
  Position GetNamePos() const { return m_Ident->GetPos(); }
  Position GetPos() const { return m_Ident->GetPos().MergeWith(m_AstType->GetPos()); }

private:
  Ptr<IdentExpr> m_Ident;
  Ptr<AstType> m_AstType;
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
  FunParams(Position position, std::vector<FunParam> params, std::optional<Ellipsis> varArgsNotation) : m_Pos(position), m_Params(std::move(params)), m_Ellipsis(varArgsNotation) {};

  Position GetPos() const { return m_Pos; }
  std::vector<FunParam> GetParams() const { return m_Params; }
  bool IsVarArgs() const { return m_Ellipsis.has_value(); }

private:
  Position m_Pos;
  std::vector<FunParam> m_Params;
  std::optional<Ellipsis> m_Ellipsis;
};

class FunSign
{
public:
  FunSign(bool isPub, Position pos, Ptr<IdentExpr> ident, FunParams params, Ptr<AstType> retType) : m_IsPub(isPub), m_Pos(pos), m_Ident(ident), m_Params(params), m_RetType(retType) {};

  Position GetPos() const { return m_Pos; }
  Position GetNamePos() const { return m_Ident->GetPos(); }
  Position GetParamsPos() const { return m_Params.GetPos(); }
  bool IsPub() const { return m_IsPub; }
  bool IsVarArgs() const { return m_Params.IsVarArgs(); }
  std::string GetName() const { return m_Ident->GetValue(); }
  std::vector<FunParam> GetParams() const { return m_Params.GetParams(); }
  Ptr<AstType> GetRetType() const { return m_RetType; }

private:
  bool m_IsPub;
  Position m_Pos;
  Ptr<IdentExpr> m_Ident;
  FunParams m_Params;
  Ptr<AstType> m_RetType;
};

class FunStmt : public Stmt
{
public:
  FunStmt(FunSign sign, Ptr<BlockStmt> body) : Stmt(StmtT::Fun), m_Sign(sign), m_Body(body) {};

  Position GetPos() const override { return m_Sign.GetPos().MergeWith(m_Body->GetPos()); }
  Ptr<BlockStmt> GetBody() const { return m_Body; }
  FunSign GetSign() const { return m_Sign; }

private:
  FunSign m_Sign;
  Ptr<BlockStmt> m_Body;
};

class LetStmt : public Stmt
{
public:
  LetStmt(bool isPub, Position pos, Ptr<IdentExpr> ident, Ptr<AstType> astType, Ptr<Expr> init) : Stmt(StmtT::Let), m_IsPub(isPub), m_Pos(pos), m_Ident(ident), m_AstType(astType), m_Init(init) {};

  bool IsPub() const { return m_IsPub; }
  Position GetPos() const override { return m_Pos; }
  Position GetNamePos() const { return m_Ident->GetPos(); }
  std::string GetName() const { return m_Ident->GetValue(); }
  Ptr<AstType> GetAstType() const { return m_AstType; }
  Ptr<Expr> GetInit() const { return m_Init; }

private:
  bool m_IsPub;
  Position m_Pos;
  Ptr<IdentExpr> m_Ident;
  Ptr<AstType> m_AstType;
  Ptr<Expr> m_Init;
};

class ImportStmt : public Stmt
{
public:
  ImportStmt(Position pos, Ptr<IdentExpr> alias, std::optional<Token> atToken, std::vector<Ptr<IdentExpr>> path) : Stmt(StmtT::Import), m_Pos(pos), m_Name(alias), m_AtToken(atToken), m_Path(std::move(path)) {};

  Position GetPos() const override { return m_Pos.MergeWith(m_Path.back()->GetPos()); }
  Position GetNamePos() const { return m_Name->GetPos(); }
  Position GetPathPos() const { return m_Path.front()->GetPos().MergeWith(m_Path.back()->GetPos()); }
  std::string GetName() const { return m_Name->GetValue(); }
  std::vector<Ptr<IdentExpr>> GetPath() const { return m_Path; }
  bool hasAtNotation() const { return m_AtToken.has_value(); }

private:
  Position m_Pos; // `import` token position
  Ptr<IdentExpr> m_Name;
  std::optional<Token> m_AtToken;
  std::vector<Ptr<IdentExpr>> m_Path;
};

class Ast
{
public:
  std::vector<Ptr<Stmt>> m_Program;

  Ast() : m_Program() {};

  std::string Inspect();
};
