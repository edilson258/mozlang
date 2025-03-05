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

  void InspectStatement(std::shared_ptr<Statement>);
  void InspectStatementBlock(std::shared_ptr<StatementBlock>);
  void InspectStatementReturn(std::shared_ptr<StatementReturn>);
  void InspectStatementLet(std::shared_ptr<StatementLet>);
  void InspectStatementFunction(std::shared_ptr<StatementFunction>);
  void InspectExpression(std::shared_ptr<Expression>);
  void InspectExpressionCall(std::shared_ptr<ExpressionCall>);
  void InspectExpressionAssign(std::shared_ptr<ExpressionAssign>);
  void InspectExpressionFieldAccess(std::shared_ptr<ExpressionFieldAccess>);
  void InspectExpressionString(std::shared_ptr<ExpressionString>);
  void InspectEpressionIdentifier(std::shared_ptr<ExpressionIdentifier>);
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

  for (std::shared_ptr<Statement> stmt : Ast.m_Program)
  {
    InspectStatement(stmt);
  }

  return Output.str();
}

void ASTInspector::InspectStatement(std::shared_ptr<Statement> statement)
{
  switch (statement.get()->m_Type)
  {
  case StatementType::IMPORT:
    break;
  case StatementType::LET:
    return InspectStatementLet(std::static_pointer_cast<StatementLet>(statement));
  case StatementType::BLOCK:
    return InspectStatementBlock(std::static_pointer_cast<StatementBlock>(statement));
  case StatementType::RETURN:
    return InspectStatementReturn(std::static_pointer_cast<StatementReturn>(statement));
  case StatementType::FUNCTION:
    return InspectStatementFunction(std::static_pointer_cast<StatementFunction>(statement));
  case StatementType ::EXPRESSION:
    return InspectExpression(std::static_pointer_cast<Expression>(statement));
  }
}

void ASTInspector::InspectStatementLet(std::shared_ptr<StatementLet> letStatement)
{
  Writeln("Let Statement:");
  Tab();
  Writeln(std::format("Name: {}", letStatement.get()->m_Identifier.get()->m_Value));
  Writeln("Value:");
  Tab();
  if (letStatement.get()->m_Initializer.has_value())
  {
    InspectExpression(letStatement.get()->m_Initializer.value());
  }
  else
  {
    Writeln("<no value>");
  }
  UnTab();
  UnTab();
}

void ASTInspector::InspectStatementBlock(std::shared_ptr<StatementBlock> blockStatement)
{
  Writeln("Block Statement:");
  Tab();
  for (auto &statement : blockStatement.get()->m_Stmts)
  {
    InspectStatement(statement);
  }
  UnTab();
}

void ASTInspector::InspectStatementReturn(std::shared_ptr<StatementReturn> returnStatement)
{
  Writeln("Return Statement:");
  Tab();
  if (returnStatement.get()->m_Value.has_value())
  {
    InspectExpression(returnStatement.get()->m_Value.value());
  }
  UnTab();
}

void ASTInspector::InspectStatementFunction(std::shared_ptr<StatementFunction> functionStatement)
{
  Writeln("Function Statement:");
  Tab();
  Writeln(std::format("Name: {}", functionStatement.get()->m_Identifier.get()->m_Value));
  InspectStatementBlock(functionStatement.get()->m_Body);
  UnTab();
}

void ASTInspector::InspectExpression(std::shared_ptr<Expression> expression)
{
  switch (expression.get()->m_Type)
  {
  case ExpressionType::FIELD_ACCESS:
    return InspectExpressionFieldAccess(std::static_pointer_cast<ExpressionFieldAccess>(expression));
  case ExpressionType::ASSIGN:
    return InspectExpressionAssign(std::static_pointer_cast<ExpressionAssign>(expression));
  case ExpressionType::CALL:
    return InspectExpressionCall(std::static_pointer_cast<ExpressionCall>(expression));
  case ExpressionType::STRING:
    return InspectExpressionString(std::static_pointer_cast<ExpressionString>(expression));
  case ExpressionType::IDENTIFIER:
    return InspectEpressionIdentifier(std::static_pointer_cast<ExpressionIdentifier>(expression));
  }
}

void ASTInspector::InspectExpressionCall(std::shared_ptr<ExpressionCall> callExpression)
{
  Position position = callExpression.get()->m_Position;
  Writeln(std::format("call expression: {{{}:{}:{}:{}}}", position.m_Line, position.m_Column, position.m_Start, position.m_End));
  Tab();

  Writeln("callee:");
  Tab();
  InspectExpression(callExpression.get()->m_Callee);
  UnTab();

  Writeln("arguments: [");
  Tab();
  for (auto argument : callExpression.get()->m_Arguments)
  {
    InspectExpression(argument);
  }
  UnTab();
  Writeln("]");

  UnTab();
}

void ASTInspector::InspectExpressionAssign(std::shared_ptr<ExpressionAssign> assignExpression)
{
  Writeln("Assign Expression:");
  Tab();
  Writeln(std::format("Assignee: {}", assignExpression.get()->m_Assignee.get()->m_Value));
  Writeln("Value:");
  Tab();
  InspectExpression(assignExpression.get()->m_Value);
  UnTab();
  UnTab();
}

void ASTInspector::InspectExpressionFieldAccess(std::shared_ptr<ExpressionFieldAccess> fieldAccessExpression)
{
  Position pos = fieldAccessExpression.get()->m_Position;
  Writeln(std::format("Field Access Expression: {}:{}:{}:{}", pos.m_Line, pos.m_Column, pos.m_Start, pos.m_End));
  Tab();
  Writeln(std::format("Field Name: {}", fieldAccessExpression.get()->m_FieldName.get()->m_Value));
  Writeln("Value:");
  Tab();
  InspectExpression(fieldAccessExpression.get()->m_Value);
  UnTab();
  UnTab();
}

void ASTInspector::InspectExpressionString(std::shared_ptr<ExpressionString> stringExpression)
{
  Writeln(std::format("string literal: {}", stringExpression.get()->m_Value));
}

// ;
void ASTInspector::InspectEpressionIdentifier(std::shared_ptr<ExpressionIdentifier> identifierExpression)
{
  Writeln(std::format("identifer expression: {}", identifierExpression.get()->m_Value));
}

std::string AST::Inspect()
{
  ASTInspector astInspector(*this);
  return astInspector.Inspect();
}
