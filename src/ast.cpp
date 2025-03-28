#include <cstddef>
#include <format>
#include <sstream>
#include <string>

#include "ast.h"
#include "token.h"

class ASTInspector
{
public:
  ASTInspector(AST &ast) : Ast(ast), TabRate(4), TabSize(0), Output() {}

  std::string Inspect();

private:
  AST &Ast;

  size_t TabRate;
  size_t TabSize;
  std::ostringstream Output;

  void Tab();
  void UnTab();
  void Write(std::string);
  void Writeln(std::string);

  void InspectStmt(Stmt *);
  void InspectBlockStmt(BlockStmt *);
  void InspectStatementReturn(RetStmt *);
  void InspectStatementLet(LetStmt *);
  void InspectStatementImport(ImportStmt *);
  void InspectStatementFunction(FunStmt *);
  void InspectExpression(Expr *);
  void InspectExpressionCall(CallExpr *);
  void InspectExpressionAssign(AssignExpr *);
  void InspectExpressionFieldAccess(FieldAccExpr *);
  void InspectExpressionString(StringExpr *);
  void InspectEpressionIdentifier(IdentExpr *);
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

  for (auto *stmt : Ast.m_Program)
  {
    InspectStmt(stmt);
  }

  return Output.str();
}

void ASTInspector::InspectStmt(Stmt *stmt)
{
  switch (stmt->GetType())
  {
  case StmtT::Import:
    return InspectStatementImport((ImportStmt *)stmt);
  case StmtT::Let:
    return InspectStatementLet((LetStmt *)stmt);
  case StmtT::Block:
    return InspectBlockStmt((BlockStmt *)stmt);
  case StmtT::Ret:
    return InspectStatementReturn((RetStmt *)stmt);
  case StmtT::Fun:
    return InspectStatementFunction((FunStmt *)stmt);
  case StmtT ::Expr:
    return InspectExpression((Expr *)stmt);
  }
}

void ASTInspector::InspectStatementLet(LetStmt *letStatement)
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

void ASTInspector::InspectStatementImport(ImportStmt *importStatement)
{
  Writeln("Import Statement:");
  Tab();
  Writeln(std::format("Name '{}'", importStatement->GetName()));
  // Writeln(std::format("Path '{}'", importStatement.get()->GetPath()));
  UnTab();
}

void ASTInspector::InspectBlockStmt(BlockStmt *blockStmt)
{
  Writeln("Block Statement:");
  Tab();
  for (auto &stmt : blockStmt->GetStatements())
  {
    InspectStmt(stmt);
  }
  UnTab();
}

void ASTInspector::InspectStatementReturn(RetStmt *retStmt)
{
  Writeln("Return Statement:");
  Tab();
  if (retStmt->GetValue())
  {
    InspectExpression(retStmt->GetValue());
  }
  UnTab();
}

void ASTInspector::InspectStatementFunction(FunStmt *funStmt)
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

void ASTInspector::InspectExpression(Expr *expression)
{
  switch (expression->GetType())
  {
  case ExprT::FieldAcc:
    return InspectExpressionFieldAccess((FieldAccExpr *)expression);
  case ExprT::Assign:
    return InspectExpressionAssign((AssignExpr *)expression);
  case ExprT::Call:
    return InspectExpressionCall((CallExpr *)expression);
  case ExprT::String:
    return InspectExpressionString((StringExpr *)expression);
  case ExprT::Ident:
    return InspectEpressionIdentifier((IdentExpr *)expression);
  }
}

void ASTInspector::InspectExpressionCall(CallExpr *callExpr)
{
  Position position = callExpr->GetCalleePosition();
  Writeln(std::format("call expression: {{{}:{}:{}:{}}}", position.m_Line, position.m_Column, position.m_Start, position.m_End));
  Tab();

  Writeln("callee:");
  Tab();
  InspectExpression(callExpr->GetCallee());
  UnTab();

  Writeln("arguments: [");
  Tab();
  for (auto argument : callExpr->GetArguments())
  {
    InspectExpression(argument);
  }
  UnTab();
  Writeln("]");

  UnTab();
}

void ASTInspector::InspectExpressionAssign(AssignExpr *assignExpr)
{
  Writeln("Assign Expression:");
  Tab();
  Writeln(std::format("Assignee: {}", assignExpr->GetAssignee()->GetValue()));
  Writeln("Value:");
  Tab();
  InspectExpression(assignExpr->GetValue());
  UnTab();
  UnTab();
}

void ASTInspector::InspectExpressionFieldAccess(FieldAccExpr *fieldAccessExpression)
{
  Position pos = fieldAccessExpression->GetPosition();
  Writeln(std::format("Field Access Expression: {}:{}:{}:{}", pos.m_Line, pos.m_Column, pos.m_Start, pos.m_End));
  Tab();
  Writeln(std::format("Field Name: {}", fieldAccessExpression->GetFieldName()->GetValue()));
  Writeln("Value:");
  Tab();
  InspectExpression(fieldAccessExpression->GetValue());
  UnTab();
  UnTab();
}

void ASTInspector::InspectExpressionString(StringExpr *strExpr)
{
  Writeln(std::format("string literal: {}", strExpr->GetValue()));
}

// ;
void ASTInspector::InspectEpressionIdentifier(IdentExpr *identExpr)
{
  Writeln(std::format("identifer expression: {}", identExpr->GetValue()));
}

std::string AST::Inspect()
{
  ASTInspector astInspector(*this);
  return astInspector.Inspect();
}
