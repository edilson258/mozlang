#pragma once

#include "Ast.h"
#include "DiagnosticEngine.h"
#include "Lexer.h"
#include "Token.h"

#include <vector>

enum class Precedence
{
  Lowest = 1,
  Call   = 2,
};

class Parser
{
public:
  Lexer &lexer;

  AST Parse();

  Parser(Lexer &lex, DiagnosticEngine &diagnostic) : lexer(lex), Diagnostic(diagnostic) {};

private:
  DiagnosticEngine &Diagnostic;

  Token CurrentToken;
  Token NextToken;

  void Bump();

  Precedence TokenToPrecedence(Token &);
  Precedence GetCurrentTokenPrecedence();

  // Parsers
  BlockStatement ParseBlockStatement();
  Statement *ParseStatement();
  FunctionStatement *ParseFunctionStatement();
  Expression *ParseExpressionStatement(Precedence);
  CallExpression *ParseCallExpression(Expression *);
  ReturnStatement *ParseReturnStatement();

  // Helpers
  TypeAnnotation ParseTypeAnnotation();
  TypeAnnotation ParseFunctionStatementReturnType();
  CallExpressionArgs ParseCallExpressionArgs();
  std::vector<FunctionParam> ParseFunctionStatementParams();
};
