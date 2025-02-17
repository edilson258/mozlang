#pragma once

#include "Ast.h"
#include "DiagnosticEngine.h"
#include "Lexer.h"
#include "Location.h"
#include "Token.h"

#include <filesystem>
#include <string>
#include <vector>

enum class Precedence
{
  Lowest = 1,
  Call = 2,
};

class Parser
{
public:
  Lexer &lexer;

  AST Parse();

  Parser(Lexer &lex, std::filesystem::path &filePath, std::string &fileContent) : lexer(lex), Diagnostic(DiagnosticEngine(filePath, fileContent)) {};

private:
  DiagnosticEngine Diagnostic;

  Token CurrentToken;
  Token NextToken;

  Location Bump();
  Location BumpExpect(TokenType, std::string);

  Precedence TokenToPrecedence(Token &);
  Precedence GetCurrentTokenPrecedence();

  // Parsers
  BlockStatement ParseBlockStatement();
  Statement *ParseStatement();
  FunctionStatement *ParseFunctionStatement();
  Expression *ParseExpressionStatement(Precedence);
  CallExpression *ParseCallExpression(Expression *);
  ReturnStatement *ParseReturnStatement();
  IdentifierExpression *ParseIdentifierExpression();

  // Helpers
  TypeAnnotation ParseTypeAnnotation();
  TypeAnnotation ParseFunctionStatementReturnType();
  CallExpressionArgs ParseCallExpressionArgs();
  FunctionParam ParseFunctionStatementParam();
  std::vector<FunctionParam> ParseFunctionStatementParams();
};
