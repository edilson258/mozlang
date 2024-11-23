#include "Parser.h"
#include "Ast.h"
#include "DiagnosticEngine.h"
#include "Location.h"
#include "Token.h"
#include "TypeSystem.h"

#include <cstdlib>
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

void Parser::Bump()
{
  CurrentToken = NextToken;
  NextToken    = lexer.GetNextToken();
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

  Expression *expression = ParseExpressionStatement(Precedence::Lowest);
  if (TokenType::Semicolon != CurrentToken.Type)
  {
    Diagnostic.Error(ErrorCode::UnexpectedToken, "Expressions must end with ';'.", expression->Loc);
    std::exit(1);
  }
  Bump();
  return expression;
}

FunctionStatement *Parser::ParseFunctionStatement()
{
  Location loc = CurrentToken.Loc;
  Bump(); // eat 'fn'

  if (CurrentToken.Type != TokenType::Identifier)
  {
    Diagnostic.Error(ErrorCode::UnexpectedToken, "Expect function's name after 'fn' keyword.", CurrentToken.Loc);
    std::exit(1);
  }

  auto *identifier = new IdentifierExpression(CurrentToken);
  Bump(); // eat function's name

  auto params               = ParseFunctionStatementParams();
  TypeAnnotation returnType = ParseFunctionStatementReturnType();
  BlockStatement body       = ParseBlockStatement();

  loc.End = body.Loc.End;
  return new FunctionStatement(loc, identifier, body, returnType, params);
}

std::vector<FunctionParam> Parser::ParseFunctionStatementParams()
{
  if (TokenType::LeftParent != CurrentToken.Type)
  {
    Diagnostic.Error(ErrorCode::UnexpectedToken, "Expect '(' after function's name.", CurrentToken.Loc);
    std::exit(1);
  }
  Bump();

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
    Diagnostic.Error(ErrorCode::UnexpectedToken, "Invalid parameter's name.", CurrentToken.Loc);
    std::exit(1);
  }
  Location loc     = CurrentToken.Loc;
  auto *identifier = new IdentifierExpression(CurrentToken);
  Bump();
  TypeAnnotation type = ParseTypeAnnotation();
  loc.End             = CurrentToken.Loc.End;
  Bump();
  params.push_back(FunctionParam(loc, identifier, type));

  // case 3: (x: int, y: str, ...)
  while (TokenType::RightParent != CurrentToken.Type)
  {
    if (TokenType::Eof == CurrentToken.Type)
    {
      Diagnostic.Error(ErrorCode::UnexpectedToken, "Unexpected EOF.", CurrentToken.Loc);
      std::exit(1);
    }

    if (TokenType::Comma != CurrentToken.Type)
    {
      Diagnostic.Error(ErrorCode::UnexpectedToken, "Expect `)` or `,` to separate function's params.",
                       CurrentToken.Loc);
      std::exit(1);
    }

    Bump(); // eat `,`

    if (TokenType::Identifier != CurrentToken.Type)
    {
      Diagnostic.Error(ErrorCode::UnexpectedToken, "Invalid parameter's name.", CurrentToken.Loc);
      std::exit(1);
    }
    loc        = CurrentToken.Loc;
    identifier = new IdentifierExpression(CurrentToken);
    Bump();
    type    = ParseTypeAnnotation();
    loc.End = CurrentToken.Loc.End;
    Bump();
    params.push_back(FunctionParam(loc, identifier, type));
  }

  Bump(); // eat `)`

  return params;
}

TypeAnnotation Parser::ParseFunctionStatementReturnType()
{
  if (TokenType::LeftBrace == CurrentToken.Type)
  {
    return TypeAnnotation(new Type(BaseType::Void));
  }

  TypeAnnotation type = ParseTypeAnnotation();
  Bump();
  return type;
}

ReturnStatement *Parser::ParseReturnStatement()
{
  Location loc = CurrentToken.Loc;
  Bump(); // eat 'return'

  if (TokenType::Semicolon == CurrentToken.Type)
  {
    loc.End = CurrentToken.Loc.End;
    Bump(); // eat ';'
    return new ReturnStatement(loc);
  }

  Expression *value = ParseExpressionStatement(Precedence::Lowest);
  if (TokenType::Semicolon != CurrentToken.Type)
  {
    Diagnostic.Error(ErrorCode::UnexpectedToken, "Expressions must end with ';'.", CurrentToken.Loc);
    std::exit(1);
  }
  loc.End = CurrentToken.Loc.End;
  Bump();
  return new ReturnStatement(loc, value);
}

BlockStatement Parser::ParseBlockStatement()
{
  if (TokenType::LeftBrace != CurrentToken.Type)
  {
    Diagnostic.Error(ErrorCode::UnexpectedToken, "Expect '{' to open block.", CurrentToken.Loc);
    std::exit(1);
  }
  Location loc = CurrentToken.Loc;
  Bump();

  std::vector<Statement *> block;

  while (1)
  {
    if (CurrentToken.Type == TokenType::Eof)
    {
      Diagnostic.Error(ErrorCode::UnexpectedToken, "Unexpected EOF.", CurrentToken.Loc);
      std::exit(1);
    }

    if (CurrentToken.Type == TokenType::RightBrace)
    {
      break;
    }

    block.push_back(ParseStatement());
  }

  loc.End = CurrentToken.Loc.End;
  Bump(); // eat '}'

  return BlockStatement(loc, block);
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
  else if (TokenType::True == CurrentToken.Type)
  {
    leftHandSide = new BooleanExpression(CurrentToken, true);
  }
  else if (TokenType::False == CurrentToken.Type)
  {
    leftHandSide = new BooleanExpression(CurrentToken, false);
  }
  else
  {
    Diagnostic.Error(ErrorCode::UnexpectedToken, "Unexpected expression.", CurrentToken.Loc);
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
  CallExpressionArgs args = ParseCallExpressionArgs();
  Location loc            = callee->Loc;
  loc.End                 = args.Loc.End;
  return new CallExpression(loc, callee, args);
}

CallExpressionArgs Parser::ParseCallExpressionArgs()
{
  Location loc = CurrentToken.Loc;
  Bump();

  // case 1: ()
  if (TokenType::RightParent == CurrentToken.Type)
  {
    loc.End = CurrentToken.Loc.End;
    Bump();
    return CallExpressionArgs(loc, {});
  }

  std::vector<Expression *> args;

  // case 2: (x)
  args.push_back(ParseExpressionStatement(Precedence::Lowest));

  // case 3: (x, y, ...)
  while (TokenType::RightParent != CurrentToken.Type)
  {
    if (TokenType::Eof == CurrentToken.Type)
    {
      Diagnostic.Error(ErrorCode::UnexpectedToken, "Unexpected EOF.", CurrentToken.Loc);
      std::exit(1);
    }

    if (TokenType::Comma != CurrentToken.Type)
    {
      Diagnostic.Error(ErrorCode::UnexpectedToken, "Expect `)` or `,` to separate function's arguments.",
                       CurrentToken.Loc);
      std::exit(1);
    }

    Bump(); // eat `,`
    args.push_back(ParseExpressionStatement(Precedence::Lowest));
  }

  loc.End = CurrentToken.Loc.End;
  Bump();

  return CallExpressionArgs(loc, args);
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
  if (TokenType::Colon != CurrentToken.Type)
  {
    Diagnostic.Error(ErrorCode::UnexpectedToken, "Expect ':' before type specifier.", CurrentToken.Loc);
    std::exit(1);
  }
  Bump();

  if (TokenType::TypeInt == CurrentToken.Type)
  {
    auto typeAnnotation = TypeAnnotation(new Type(BaseType::Integer), CurrentToken);
    return typeAnnotation;
  }

  if (TokenType::TypeStr == CurrentToken.Type)
  {
    auto typeAnnotation = TypeAnnotation(new Type(BaseType::String), CurrentToken);
    return typeAnnotation;
  }

  if (TokenType::TypeVoid == CurrentToken.Type)
  {
    auto typeAnnotation = TypeAnnotation(new Type(BaseType::Void), CurrentToken);
    return typeAnnotation;
  }

  if (TokenType::TypeBool == CurrentToken.Type)
  {
    auto typeAnnotation = TypeAnnotation(new Type(BaseType::Boolean), CurrentToken);
    return typeAnnotation;
  }

  Diagnostic.Error(ErrorCode::UnexpectedToken, "Invalid type annotation", CurrentToken.Loc);
  std::exit(1);
}
