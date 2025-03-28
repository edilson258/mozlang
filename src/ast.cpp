#include <cstddef>
#include <format>
#include <sstream>
#include <string>

#include "ast.h"
#include "pointer.h"
#include "token.h"

class ASTInspector
{
public:
  ASTInspector(Ast &ast) : m_Ast(ast), TabRate(4), TabSize(0), Output() {}

  std::string Inspect();

private:
  Ast &m_Ast;

  size_t TabRate;
  size_t TabSize;
  std::ostringstream Output;

  void Tab();
  void UnTab();
  void Write(std::string);
  void Writeln(std::string);

  void InspectStmt(Ptr<Stmt>);
  void InspectBlockStmt(Ptr<BlockStmt>);
  void InspectStatementReturn(Ptr<RetStmt>);
  void InspectStatementLet(Ptr<LetStmt>);
  void InspectStatementImport(Ptr<ImportStmt>);
  void InspectStatementFunction(Ptr<FunStmt>);
  void InspectExpression(Ptr<Expr>);
  void InspectExpressionCall(Ptr<CallExpr>);
  void InspectExpressionAssign(Ptr<AssignExpr>);
  void InspectExpressionFieldAccess(Ptr<FieldAccExpr>);
  void InspectExpressionString(Ptr<StringExpr>);
  void InspectEpressionIdentifier(Ptr<IdentExpr>);
};

void ASTInspector::Tab()
{
  TabSize += TabRate;
}

void ASTInspector::UnTab()
{
  TabSize -= TabRate;
}

void ASTInspector::Write(std::string str)
{
  Output << std::string(TabSize, ' ') << str;
}

void ASTInspector::Writeln(std::string str)
{
  Write(str);
  Output << '\n';
}

std::string ASTInspector::Inspect()
{
  Output << "Abstract Syntax Tree\n\n";

  for (auto stmt : m_Ast.m_Program)
  {
    InspectStmt(stmt);
  }

  return Output.str();
}

void ASTInspector::InspectStmt(Ptr<Stmt> stmt)
{
  switch (stmt->GetType())
  {
  case StmtT::Import:
    return InspectStatementImport(CastPtr<ImportStmt>(stmt));
  case StmtT::Let:
    return InspectStatementLet(CastPtr<LetStmt>(stmt));
  case StmtT::Block:
    return InspectBlockStmt(CastPtr<BlockStmt>(stmt));
  case StmtT::Ret:
    return InspectStatementReturn(CastPtr<RetStmt>(stmt));
  case StmtT::Fun:
    return InspectStatementFunction(CastPtr<FunStmt>(stmt));
  case StmtT::Expr:
    return InspectExpression(CastPtr<Expr>(stmt));
  }
}

void ASTInspector::InspectStatementLet(Ptr<LetStmt> letStatement)
{
  Writeln("Let Statement:");
  Tab();
  Writeln(std::format("Is Pub: {}", letStatement->IsPub()));
  Writeln(std::format("Name: {}", letStatement->GetName()));
  Writeln("Value:");
  Tab();
  if (letStatement->GetInit())
  {
    InspectExpression(letStatement->GetInit());
  }
  else
  {
    Writeln("<no value>");
  }
  UnTab();
  UnTab();
}

void ASTInspector::InspectStatementImport(Ptr<ImportStmt> importStatement)
{
  Writeln("Import Statement:");
  Tab();
  Writeln(std::format("Name '{}'", importStatement->GetName()));
  // Writeln(std::format("Path '{}'", importStatement.get()->GetPath()));
  UnTab();
}

void ASTInspector::InspectBlockStmt(Ptr<BlockStmt> blockStmt)
{
  Writeln("Block Statement:");
  Tab();
  for (auto &stmt : blockStmt->GetStatements())
  {
    InspectStmt(stmt);
  }
  UnTab();
}

void ASTInspector::InspectStatementReturn(Ptr<RetStmt> retStmt)
{
  Writeln("Return Statement:");
  Tab();
  if (retStmt->GetValue())
  {
    InspectExpression(retStmt->GetValue());
  }
  UnTab();
}

void ASTInspector::InspectStatementFunction(Ptr<FunStmt> funStmt)
{
  Writeln("Function Statement:");
  Tab();
  Writeln("Signature:");
  Tab();
  auto funSign = funStmt->GetSign();
  Writeln(std::format("Is Pub: {}", funSign.IsPub()));
  Writeln(std::format("Name: {}", funSign.GetName()));
  Writeln(std::format("Return type: {}", funSign.GetRetType() ? funSign.GetRetType()->GetType()->Inspect() : "void"));
  Writeln("Parameters: [");
  Tab();
  for (auto &param : funSign.GetParams())
  {
    Writeln(std::format("Name: {} Type: {}", param.GetName(), param.GetAstType()->GetType()->Inspect()));
  }
  UnTab();
  Writeln("]");
  UnTab();
  if (funStmt->GetBody())
  {
    InspectBlockStmt(funStmt->GetBody());
  }
  UnTab();
}

void ASTInspector::InspectExpression(Ptr<Expr> expression)
{
  switch (expression->GetType())
  {
  case ExprT::Assign:
    return InspectExpressionAssign(CastPtr<AssignExpr>(expression));
  case ExprT::Call:
    return InspectExpressionCall(CastPtr<CallExpr>(expression));
  case ExprT::String:
    return InspectExpressionString(CastPtr<StringExpr>(expression));
  case ExprT::Ident:
    return InspectEpressionIdentifier(CastPtr<IdentExpr>(expression));
  case ExprT::FieldAcc:
    return InspectExpressionFieldAccess(CastPtr<FieldAccExpr>(expression));
  }
}

void ASTInspector::InspectExpressionCall(Ptr<CallExpr> callExpr)
{
  Position position = callExpr->GetCalleePos();
  Writeln(std::format("call expression: {{{}:{}:{}:{}}}", position.m_Line, position.m_Column, position.m_Start, position.m_End));
  Tab();

  Writeln("callee:");
  Tab();
  InspectExpression(callExpr->GetCallee());
  UnTab();

  Writeln("arguments: [");
  Tab();
  for (auto argument : callExpr->GetArgs())
  {
    InspectExpression(argument);
  }
  UnTab();
  Writeln("]");

  UnTab();
}

void ASTInspector::InspectExpressionAssign(Ptr<AssignExpr> assignExpr)
{
  Writeln("Assign Expression:");
  Tab();
  Writeln(std::format("Assignee: {}", assignExpr->GetDest()->GetValue()));
  Writeln("Value:");
  Tab();
  InspectExpression(assignExpr->GetValue());
  UnTab();
  UnTab();
}

void ASTInspector::InspectExpressionFieldAccess(Ptr<FieldAccExpr> fieldAccessExpression)
{
  Position pos = fieldAccessExpression->GetPos();
  Writeln(std::format("Field Access Expression: {}:{}:{}:{}", pos.m_Line, pos.m_Column, pos.m_Start, pos.m_End));
  Tab();
  Writeln(std::format("Field Name: {}", fieldAccessExpression->GetFieldName()->GetValue()));
  Writeln("Value:");
  Tab();
  InspectExpression(fieldAccessExpression->GetValue());
  UnTab();
  UnTab();
}

void ASTInspector::InspectExpressionString(Ptr<StringExpr> strExpr)
{
  Writeln(std::format("string literal: {}", strExpr->GetValue()));
}

void ASTInspector::InspectEpressionIdentifier(Ptr<IdentExpr> identExpr)
{
  Writeln(std::format("identifer expression: {}", identExpr->GetValue()));
}

std::string Ast::Inspect()
{
  ASTInspector astInspector(*this);
  return astInspector.Inspect();
}
