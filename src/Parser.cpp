#include "Parser.h"
#include "Ast.h"
#include "Token.h"
#include "TypeSystem.h"

#include <cstdlib>
#include <iostream>
#include <vector>

AST Parser::Parse()
{
  Bump();
  Bump();

  AST ast(lexer.FilePath);

  while (TokenType::Eof != CurrentToken.Type)
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

  std::vector<FunctionParam> params = ParseFunctionStatementParams();
  TypeAnnotation returnType         = ParseFunctionStatementReturnType();
  BlockStatement body               = ParseBlockStatement();

  return new FunctionStatement(identifier, body, returnType, params);
}

std::vector<FunctionParam> Parser::ParseFunctionStatementParams()
{
  BumpExpected(TokenType::LeftParent);

  // case 1: ()
  if (TokenType::RightParent == CurrentToken.Type)
  {
    Bump();
    return {};
  }

  std::vector<FunctionParam> params;

  // case 2: (x: int)
  if (TokenType::Identifier != CurrentToken.Type)
  {
    std::cerr << "[ERROR]: Expected parameter name or `)` but got " << CurrentToken.ToString() << std::endl;
    std::exit(1);
  }

  auto *identifier = new IdentifierExpression(CurrentToken);
  Bump();
  TypeAnnotation type = ParseTypeAnnotation();
  params.push_back(FunctionParam(identifier, type));

  // case 3: (x: int, y: str, ...)
  while (TokenType::RightParent != CurrentToken.Type)
  {
    if (TokenType::Eof == CurrentToken.Type)
    {
      std::cerr << "[ERROR]: Early EOF\n";
      std::exit(1);
    }

    if (TokenType::Comma != CurrentToken.Type)
    {
      std::cerr << "[ERROR]: Expect `)` or `,` to separate fn params but got " << CurrentToken.ToString() << std::endl;
      std::exit(1);
    }

    Bump(); // eat `,`

    identifier = new IdentifierExpression(CurrentToken);
    Bump();
    type = ParseTypeAnnotation();
    params.push_back(FunctionParam(identifier, type));
  }

  Bump();

  return params;
}

TypeAnnotation Parser::ParseFunctionStatementReturnType()
{
  if (TokenType::LeftBrace == CurrentToken.Type)
  {
    return TypeAnnotation(new Type(BaseType::Void));
  }

  return ParseTypeAnnotation();
}

ReturnStatement *Parser::ParseReturnStatement()
{
  Token returnLexeme = CurrentToken;
  Bump(); // eat 'return'

  if (TokenType::Semicolon == CurrentToken.Type)
  {
    Bump(); // eat ';'
    return new ReturnStatement(returnLexeme);
  }

  Expression *value = ParseExpressionStatement(Precedence::Lowest);
  BumpExpected(TokenType::Semicolon);
  return new ReturnStatement(returnLexeme, value);
}

BlockStatement Parser::ParseBlockStatement()
{
  BumpExpected(TokenType::LeftBrace);

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

TypeAnnotation Parser::ParseTypeAnnotation()
{
  BumpExpected(TokenType::Colon);

  if (TokenType::TypeInt == CurrentToken.Type)
  {
    auto typeAnnotation = TypeAnnotation(new Type(BaseType::Integer), CurrentToken);
    Bump();
    return typeAnnotation;
  }

  if (TokenType::TypeStr == CurrentToken.Type)
  {
    auto typeAnnotation = TypeAnnotation(new Type(BaseType::String), CurrentToken);
    Bump();
    return typeAnnotation;
  }

  if (TokenType::TypeVoid == CurrentToken.Type)
  {
    auto typeAnnotation = TypeAnnotation(new Type(BaseType::Void), CurrentToken);
    Bump();
    return typeAnnotation;
  }

  std::cerr << "[ERROR] Expect type annotation but got " << CurrentToken.ToString() << std::endl;
  std::exit(1);
}
