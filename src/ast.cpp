#include <cstddef>
#include <format>
#include <sstream>
#include <string>

#include "ast.h"

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
  void InspectExpression(std::shared_ptr<Expression>);
  void InspectExpressionCall(std::shared_ptr<ExpressionCall>);
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

  for (std::shared_ptr<Statement> stmt : Ast.Program)
  {
    InspectStatement(stmt);
  }

  return Output.str();
}

void ASTInspector::InspectStatement(std::shared_ptr<Statement> statement)
{
  switch (statement.get()->Type)
  {
  case StatementType ::EXPRESSION:
    InspectExpression(std::static_pointer_cast<Expression>(statement));
  }
}

void ASTInspector::InspectExpression(std::shared_ptr<Expression> expression)
{
  switch (expression.get()->Type)
  {
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
  Position position = callExpression.get()->Pos;
  Writeln(std::format("call expression: {{{}:{}:{}:{}}}", position.Line, position.Column, position.Start, position.End));
  Tab();

  Writeln("callee:");
  Tab();
  InspectExpression(callExpression.get()->Callee);
  UnTab();

  Writeln("arguments: [");
  Tab();
  for (auto argument : callExpression.get()->Arguments)
  {
    InspectExpression(argument);
  }
  UnTab();
  Writeln("]");

  UnTab();
}

void ASTInspector::InspectExpressionString(std::shared_ptr<ExpressionString> stringExpression)
{
  Writeln(std::format("string literal: {}", stringExpression.get()->Value));
}

// ;
void ASTInspector::InspectEpressionIdentifier(std::shared_ptr<ExpressionIdentifier> identifierExpression)
{
  Writeln(std::format("identifer expression: {}", identifierExpression.get()->Value));
}

std::string AST::Inspect()
{
  ASTInspector astInspector(*this);
  return astInspector.Inspect();
}
