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
  void InspectStatementImport(std::shared_ptr<StatementImport>);
  void InspectStatementFunctionSignature(std::shared_ptr<StatementFunctionSignature>);
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
  switch (statement.get()->GetType())
  {
  case StatementType::FUNCTION_SIGNATURE:
    return InspectStatementFunctionSignature(std::static_pointer_cast<StatementFunctionSignature>(statement));
  case StatementType::IMPORT:
    return InspectStatementImport(std::static_pointer_cast<StatementImport>(statement));
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
  Writeln(std::format("Name: {}", letStatement.get()->GetName()));
  Writeln("Value:");
  Tab();
  if (letStatement.get()->GetInitializer().has_value())
  {
    InspectExpression(letStatement.get()->GetInitializer().value());
  }
  else
  {
    Writeln("<no value>");
  }
  UnTab();
  UnTab();
}

void ASTInspector::InspectStatementImport(std::shared_ptr<StatementImport> importStatement)
{
  Writeln("Import Statement:");
  Tab();
  Writeln(std::format("Alias '{}'", importStatement.get()->GetName()));
  Writeln(std::format("Path '{}'", importStatement.get()->GetPath()));
  UnTab();
}

void ASTInspector::InspectStatementBlock(std::shared_ptr<StatementBlock> blockStatement)
{
  Writeln("Block Statement:");
  Tab();
  for (auto &statement : blockStatement.get()->GetStatements())
  {
    InspectStatement(statement);
  }
  UnTab();
}

void ASTInspector::InspectStatementReturn(std::shared_ptr<StatementReturn> returnStatement)
{
  Writeln("Return Statement:");
  Tab();
  if (returnStatement.get()->GetValue().has_value())
  {
    InspectExpression(returnStatement.get()->GetValue().value());
  }
  UnTab();
}

void ASTInspector::InspectStatementFunctionSignature(std::shared_ptr<StatementFunctionSignature> functionSignature)
{
  Writeln("Function Signature:");
  Tab();
  Writeln(std::format("Name: {}", functionSignature.get()->GetName()));
  Writeln(std::format("Return type: {}", functionSignature.get()->GetReturnType().has_value() ? functionSignature.get()->GetReturnType().value().GetType().get()->Inspect() : "void"));
  Writeln("Parameters: [");
  Tab();
  for (auto &param : functionSignature.get()->GetParams())
  {
    Writeln(std::format("Name: {} Type: {}", param.GetName(), param.GetAstType().has_value() ? param.GetAstType().value().GetType().get()->Inspect() : "any"));
  }
  UnTab();
  Writeln("]");
  UnTab();
}

void ASTInspector::InspectStatementFunction(std::shared_ptr<StatementFunction> functionStatement)
{
  Writeln("Function Statement:");
  Tab();
  InspectStatementFunctionSignature(functionStatement.get()->GetSignature());
  InspectStatementBlock(functionStatement.get()->GetBody());
  UnTab();
}

void ASTInspector::InspectExpression(std::shared_ptr<Expression> expression)
{
  switch (expression.get()->GetType())
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
  Position position = callExpression.get()->GetCalleePosition();
  Writeln(std::format("call expression: {{{}:{}:{}:{}}}", position.m_Line, position.m_Column, position.m_Start, position.m_End));
  Tab();

  Writeln("callee:");
  Tab();
  InspectExpression(callExpression.get()->GetCallee());
  UnTab();

  Writeln("arguments: [");
  Tab();
  for (auto argument : callExpression.get()->GetArguments())
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
  Writeln(std::format("Assignee: {}", assignExpression.get()->GetAssignee().get()->GetValue()));
  Writeln("Value:");
  Tab();
  InspectExpression(assignExpression.get()->GetValue());
  UnTab();
  UnTab();
}

void ASTInspector::InspectExpressionFieldAccess(std::shared_ptr<ExpressionFieldAccess> fieldAccessExpression)
{
  Position pos = fieldAccessExpression.get()->GetPosition();
  Writeln(std::format("Field Access Expression: {}:{}:{}:{}", pos.m_Line, pos.m_Column, pos.m_Start, pos.m_End));
  Tab();
  Writeln(std::format("Field Name: {}", fieldAccessExpression.get()->GetFieldName().get()->GetValue()));
  Writeln("Value:");
  Tab();
  InspectExpression(fieldAccessExpression.get()->GetValue());
  UnTab();
  UnTab();
}

void ASTInspector::InspectExpressionString(std::shared_ptr<ExpressionString> stringExpression)
{
  Writeln(std::format("string literal: {}", stringExpression.get()->GetValue()));
}

// ;
void ASTInspector::InspectEpressionIdentifier(std::shared_ptr<ExpressionIdentifier> identifierExpression)
{
  Writeln(std::format("identifer expression: {}", identifierExpression.get()->GetValue()));
}

std::string AST::Inspect()
{
  ASTInspector astInspector(*this);
  return astInspector.Inspect();
}
