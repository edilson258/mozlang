#include "Parser.h"
#include "Ast.h"
#include "DiagnosticEngine.h"
#include "Location.h"
#include "Token.h"
#include "TypeSystem.h"

#include <cstdlib>
#include <format>
#include <string>
#include <vector>

AST Parser::Parse()
{
  Bump();
  Bump();

  AST ast;

  while (TokenType::Eof != CurrentToken.Type)
  {
    ast.Nodes.push_back(ParseStatement());
  }

  return ast;
}

Location Parser::Bump()
{
  Location currTokenLoc = CurrentToken.Loc;
  CurrentToken = NextToken;
  NextToken = lexer.GetNextToken();
  return currTokenLoc;
}

Location Parser::BumpExpect(TokenType expected, std::string message)
{
  if (CurrentToken.Type == expected)
  {
    return Bump();
  }
  Diagnostic.ErrorAndExit(ErrorCode::UnexpectedToken, std::format("{} but got '{}'.", message, CurrentToken.Data), CurrentToken.Loc);
}

Statement *Parser::ParseStatement()
{
  switch (CurrentToken.Type)
  {
  case TokenType::Fn:
    return ParseFunctionStatement();
  case TokenType::Return:
    return ParseReturnStatement();
  default:
  {
    Expression *expression = ParseExpressionStatement(Precedence::Lowest);
    BumpExpect(TokenType::Semicolon, "Expressions must end with ';'");
    return expression;
  }
  }
}

FunctionStatement *Parser::ParseFunctionStatement()
{
  Location location = Bump();
  auto *identifier = ParseIdentifierExpression();
  auto params = ParseFunctionStatementParams();
  auto returnType = ParseFunctionStatementReturnType();
  auto body = ParseBlockStatement();
  location.End = body.Loc.End;
  return new FunctionStatement(location, identifier, body, returnType, params);
}

FunctionParam Parser::ParseFunctionStatementParam()
{
  Location location = CurrentToken.Loc;
  IdentifierExpression *identifier = ParseIdentifierExpression();
  TypeAnnotation type = ParseTypeAnnotation();
  location.End = type.Lexeme->Loc.End;
  return FunctionParam(location, identifier, type);
}

std::vector<FunctionParam> Parser::ParseFunctionStatementParams()
{
  BumpExpect(TokenType::LeftParent, "Expect '(' after function's name");

  if (TokenType::RightParent == CurrentToken.Type)
  {
    Bump();
    return {};
  }

  std::vector<FunctionParam> params;
  params.push_back(ParseFunctionStatementParam());

  while (TokenType::RightParent != CurrentToken.Type)
  {
    if (TokenType::Eof == CurrentToken.Type)
    {
      Diagnostic.ErrorAndExit(ErrorCode::UnexpectedToken, "Unexpected EOF.", CurrentToken.Loc);
    }
    BumpExpect(TokenType::Comma, "Expect `)` or `,` to separate function's params");
    params.push_back(ParseFunctionStatementParam());
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
  Location location = Bump();
  if (TokenType::Semicolon == CurrentToken.Type)
  {
    location.End = Bump().End;
    return new ReturnStatement(location);
  }
  Expression *value = ParseExpressionStatement(Precedence::Lowest);
  location.End = BumpExpect(TokenType::Semicolon, "Expressions must end with ';'").End;
  return new ReturnStatement(location, value);
}

BlockStatement Parser::ParseBlockStatement()
{
  Location location = BumpExpect(TokenType::LeftBrace, "Expect '{' to open block");
  std::vector<Statement *> block;
  while (1)
  {
    if (CurrentToken.Type == TokenType::Eof)
    {
      Diagnostic.ErrorAndExit(ErrorCode::UnexpectedToken, "Unexpected EOF.", CurrentToken.Loc);
    }
    if (CurrentToken.Type == TokenType::RightBrace)
    {
      break;
    }
    block.push_back(ParseStatement());
  }
  location.End = Bump().End;
  return BlockStatement(location, block);
}

Expression *Parser::ParseExpressionStatement(Precedence precedence)
{
  Expression *leftHandSide;
  switch (CurrentToken.Type)
  {
  case TokenType::Identifier:
    leftHandSide = new IdentifierExpression(CurrentToken);
    break;
  case TokenType::String:
    leftHandSide = new StringExpression(CurrentToken);
    break;
  case TokenType::Integer:
    leftHandSide = new IntegerExpression(CurrentToken);
    break;
  case TokenType::True:
  case TokenType::False:
    leftHandSide = new BooleanExpression(CurrentToken, CurrentToken.Type == TokenType::True ? true : false);
    break;
  default:
    Diagnostic.ErrorAndExit(ErrorCode::UnexpectedToken, "Unexpected expression.", CurrentToken.Loc);
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
  CallExpressionArgs args = ParseCallExpressionArgs();
  Location location = callee->Loc;
  location.End = args.Loc.End;
  return new CallExpression(location, callee, args);
}

CallExpressionArgs Parser::ParseCallExpressionArgs()
{
  Location location = Bump();
  if (TokenType::RightParent == CurrentToken.Type)
  {
    location.End = Bump().End;
    return CallExpressionArgs(location, {});
  }
  std::vector<Expression *> args;
  args.push_back(ParseExpressionStatement(Precedence::Lowest));
  while (TokenType::RightParent != CurrentToken.Type)
  {
    if (TokenType::Eof == CurrentToken.Type)
    {
      Diagnostic.ErrorAndExit(ErrorCode::UnexpectedToken, "Unexpected EOF.", CurrentToken.Loc);
    }
    BumpExpect(TokenType::Comma, "Expect `)` or `,` to separate function's arguments");
    args.push_back(ParseExpressionStatement(Precedence::Lowest));
  }
  location.End = Bump().End;
  return CallExpressionArgs(location, args);
}

IdentifierExpression *Parser::ParseIdentifierExpression()
{
  auto *identifier = new IdentifierExpression(CurrentToken);
  BumpExpect(TokenType::Identifier, "Expect an identifier");
  return identifier;
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
  BumpExpect(TokenType::Colon, "Expect ':' before type specifier");
  switch (CurrentToken.Type)
  {
  case TokenType::TypeInt:
    Bump();
    return TypeAnnotation(new Type(BaseType::Integer));
  case TokenType::TypeStr:
    Bump();
    return TypeAnnotation(new Type(BaseType::String));
  case TokenType::TypeVoid:
    Bump();
    return TypeAnnotation(new Type(BaseType::Void));
  case TokenType::TypeBool:
    Bump();
    return TypeAnnotation(new Type(BaseType::Boolean));
  default:
    Diagnostic.ErrorAndExit(ErrorCode::UnexpectedToken, std::format("Invalid type annotation '{}'.", CurrentToken.Data), CurrentToken.Loc);
  }
}
