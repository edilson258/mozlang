#pragma once

#include "Ast.h"
#include "Lexer.h"
#include "Token.h"

#include <memory>

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

  Parser(Lexer &lex) : lexer(lex) {};

private:
  Token CurrentToken;
  Token NextToken;

  void Bump();
  void BumpExpected(TokenType);

  Precedence TokenToPrecedence(Token &);
  Precedence GetCurrentTokenPrecedence();

  // Parsers
  BlockStatement ParseBlockStatement();
  Statement *ParseStatement();
  FunctionStatement *ParseFunctionStatement();
  Expression *ParseExpressionStatement(Precedence);
  CallExpression *ParseCallExpression(Expression *);
};