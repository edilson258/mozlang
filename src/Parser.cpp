#include "Parser.h"
#include "Ast.h"
#include "Token.h"
#include "TypeSystem.h"

#include <iostream>
#include <vector>

AST Parser::Parse()
{
  Bump();
  Bump();

  AST ast(lexer.FilePath);

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
  if (TokenType::Fn == CurrentToken.Type)
  {
    return ParseFunctionStatement();
  }

  if (TokenType::Return == CurrentToken.Type)
  {
    return ParseReturnStatement();
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

  auto *identifier = new IdentifierExpression(CurrentToken);

  Bump(); // eat function's name

  // TODO: parse params
  BumpExpected(TokenType::LeftParent);
  BumpExpected(TokenType::RightParent);

  Type returnType = ParseTypeAnnotation();

  auto body = ParseBlockStatement();

  return new FunctionStatement(identifier, body, returnType);
}

ReturnStatement *Parser::ParseReturnStatement()
{
  TokenSpan span = CurrentToken.Span;
  Bump();

  if (TokenType::Semicolon == CurrentToken.Type)
  {
    return new ReturnStatement(span);
  }

  Expression *value = ParseExpressionStatement(Precedence::Lowest);
  BumpExpected(TokenType::Semicolon);
  return new ReturnStatement(span, value);
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
  else if (TokenType::Integer == CurrentToken.Type)
  {
    leftHandSide = new IntegerExpression(CurrentToken);
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
      return leftHandSide;
    }
  }

  return leftHandSide;
}

CallExpression *Parser::ParseCallExpression(Expression *callee)
{
  return new CallExpression(callee, ParseCallExpressionArgs());
}

std::vector<Expression *> Parser::ParseCallExpressionArgs()
{
  Bump(); // eat '('

  std::vector<Expression *> args;

  while (1)
  {
    if (TokenType::Eof == CurrentToken.Type)
    {
      std::cerr << "[ERROR]: Early EOF\n";
      std::exit(1);
    }

    if (TokenType::RightParent == CurrentToken.Type)
    {
      break;
    }

    args.push_back(ParseExpressionStatement(Precedence::Lowest));

    if (CurrentToken.Type == TokenType::Comma)
    {
      Bump();
    }
  }

  Bump(); // eat ')'

  return args;
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

Type Parser::ParseTypeAnnotation()
{
  BumpExpected(TokenType::Colon);

  switch (CurrentToken.Type)
  {
  case TokenType::TypeInt:
    Bump();
    return Type(BaseType::Integer);
  case TokenType::TypeStr:
    Bump();
    return Type(BaseType::String);
  default:
    std::cerr << "[ERROR] Expect type annotation but got " << CurrentToken.ToString() << std::endl;
    std::exit(1);
  }
}
