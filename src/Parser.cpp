#include "Parser.h"

#include <format>
#include <iostream>
#include <sstream>

AST Parser::Parse()
{
  Bump();
  Bump();

  AST ast;

  while (CurrentToken.Type != TokenType::Eof)
  {
    ast.Nodes.push_back(ParseStatement());
  }

  return ast;
}

void Parser::Bump()
{
  CurrentToken = NextToken;
  NextToken    = lexer.GetNextToken();
}

void Parser::BumpExpected(TokenType expectedType)
{
  if (expectedType != CurrentToken.Type)
  {
    std::cerr << "[ERROR]: Unexpected token: " << CurrentToken.ToString() << std::endl;
    std::exit(1);
  }
  Bump();
}

Statement *Parser::ParseStatement()
{
  if (CurrentToken.Type == TokenType::Fn)
  {
    return ParseFunctionStatement();
  }

  auto expression = ParseExpressionStatement(Precedence::Lowest);
  BumpExpected(TokenType::Semicolon);
  return expression;
}

FunctionStatement *Parser::ParseFunctionStatement()
{
  Bump(); // eat 'fn'

  if (CurrentToken.Type != TokenType::Identifier)
  {
    std::cerr << "[ERROR]: Expected identifier after 'fn' but got: " << CurrentToken.ToString() << std::endl;
    std::exit(1);
  }

  auto identifier = CurrentToken;

  Bump(); // eat 'function name'

  // TODO: parse params
  BumpExpected(TokenType::LeftParent);
  BumpExpected(TokenType::RightParent);

  auto body = ParseBlockStatement();

  return new FunctionStatement(identifier, body);
}

BlockStatement Parser::ParseBlockStatement()
{
  Bump(); // eat '{'

  std::vector<Statement *> block;

  while (1)
  {
    if (CurrentToken.Type == TokenType::Eof)
    {
      std::cerr << "[ERROR]: Unexpected EOF\n";
      std::exit(1);
    }

    if (CurrentToken.Type == TokenType::RightBrace)
    {
      break;
    }

    block.push_back(ParseStatement());
  }

  Bump(); // eat '}'

  return BlockStatement(block);
}

Expression *Parser::ParseExpressionStatement(Precedence precedence)
{
  Expression *leftHandSide;

  if (CurrentToken.Type == TokenType::Identifier)
  {
    leftHandSide = new IdentifierExpression(CurrentToken);
  }
  else if (CurrentToken.Type == TokenType::String)
  {
    leftHandSide = new StringExpression(CurrentToken);
  }
  else
  {
    std::cerr << "[ERROR}: Unexpected left side expression: " << CurrentToken.ToString();
    std::exit(1);
  }

  Bump();

  while ((TokenType::Eof != CurrentToken.Type) && (precedence < GetCurrentTokenPrecedence()))
  {
    switch (CurrentToken.Type)
    {
    case TokenType::LeftParent:
      leftHandSide = ParseCallExpression(std::move(leftHandSide));
      break;
    default:
      break;
    }
  }

  return leftHandSide;
}

CallExpression *Parser::ParseCallExpression(Expression *callee)
{
  Bump(); // eat '('

  // TODO: parse args
  std::vector<Expression *> args;
  args.push_back(ParseExpressionStatement(Precedence::Call));

  BumpExpected(TokenType::RightParent);

  return new CallExpression(callee, args);
}

Precedence Parser::TokenToPrecedence(Token &t)
{
  switch (t.Type)
  {
  case TokenType::LeftParent:
    return Precedence::Call;
  default:
    return Precedence::Lowest;
  }
}

Precedence Parser::GetCurrentTokenPrecedence() { return TokenToPrecedence(CurrentToken); }