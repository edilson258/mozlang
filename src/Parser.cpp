#include "Parser.h"
#include "Ast.h"
#include "DiagnosticEngine.h"
#include "Span.h"
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
    Diagnostic.Error(ErrorCode::UnexpectedToken, "Expressions must end with ';'.", expression->Spn);
    std::exit(1);
  }
  Bump();
  return expression;
}

FunctionStatement *Parser::ParseFunctionStatement()
{
  Span spn = CurrentToken.Spn;
  Bump(); // eat 'fn'

  if (CurrentToken.Type != TokenType::Identifier)
  {
    Diagnostic.Error(ErrorCode::UnexpectedToken, "Expect function's name after 'fn' keyword.", CurrentToken.Spn);
    std::exit(1);
  }

  auto *identifier = new IdentifierExpression(CurrentToken);
  Bump(); // eat function's name

  std::vector<FunctionParam> params = ParseFunctionStatementParams();
  TypeAnnotation returnType         = ParseFunctionStatementReturnType();
  BlockStatement body               = ParseBlockStatement();

  spn.RangeEnd = body.Spn.RangeEnd;
  return new FunctionStatement(spn, identifier, body, returnType, params);
}

std::vector<FunctionParam> Parser::ParseFunctionStatementParams()
{

  if (TokenType::LeftParent != CurrentToken.Type)
  {
    Diagnostic.Error(ErrorCode::UnexpectedToken, "Expect '(' after function's name.", CurrentToken.Spn);
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
    Diagnostic.Error(ErrorCode::UnexpectedToken, "Invalid parameter's name.", CurrentToken.Spn);
    std::exit(1);
  }
  Span spn         = CurrentToken.Spn;
  auto *identifier = new IdentifierExpression(CurrentToken);
  Bump();
  TypeAnnotation type = ParseTypeAnnotation();
  spn.RangeEnd        = CurrentToken.Spn.RangeEnd;
  Bump();
  params.push_back(FunctionParam(spn, identifier, type));

  // case 3: (x: int, y: str, ...)
  while (TokenType::RightParent != CurrentToken.Type)
  {
    if (TokenType::Eof == CurrentToken.Type)
    {
      Diagnostic.Error(ErrorCode::UnexpectedToken, "Unexpected EOF.", CurrentToken.Spn);
      std::exit(1);
    }

    if (TokenType::Comma != CurrentToken.Type)
    {
      Diagnostic.Error(ErrorCode::UnexpectedToken, "Expect `)` or `,` to separate function's params.",
                       CurrentToken.Spn);
      std::exit(1);
    }

    Bump(); // eat `,`

    if (TokenType::Identifier != CurrentToken.Type)
    {
      Diagnostic.Error(ErrorCode::UnexpectedToken, "Invalid parameter's name.", CurrentToken.Spn);
      std::exit(1);
    }
    spn        = CurrentToken.Spn;
    identifier = new IdentifierExpression(CurrentToken);
    Bump();
    type         = ParseTypeAnnotation();
    spn.RangeEnd = CurrentToken.Spn.RangeEnd;
    Bump();
    params.push_back(FunctionParam(spn, identifier, type));
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

  TypeAnnotation type = ParseTypeAnnotation();
  Bump();
  return type;
}

ReturnStatement *Parser::ParseReturnStatement()
{
  Span spn = CurrentToken.Spn;
  Bump(); // eat 'return'

  if (TokenType::Semicolon == CurrentToken.Type)
  {
    spn.RangeEnd = CurrentToken.Spn.RangeEnd;
    Bump(); // eat ';'
    return new ReturnStatement(spn);
  }

  Expression *value = ParseExpressionStatement(Precedence::Lowest);
  if (TokenType::Semicolon != CurrentToken.Type)
  {
    Diagnostic.Error(ErrorCode::UnexpectedToken, "Expressions must end with ';'.", CurrentToken.Spn);
    std::exit(1);
  }
  Bump();
  return new ReturnStatement(spn, value);
}

BlockStatement Parser::ParseBlockStatement()
{
  if (TokenType::LeftBrace != CurrentToken.Type)
  {
    Diagnostic.Error(ErrorCode::UnexpectedToken, "Expect '{' to open block.", CurrentToken.Spn);
    std::exit(1);
  }
  Span spn = CurrentToken.Spn;
  Bump();

  std::vector<Statement *> block;

  while (1)
  {
    if (CurrentToken.Type == TokenType::Eof)
    {
      Diagnostic.Error(ErrorCode::UnexpectedToken, "Unexpected EOF.", CurrentToken.Spn);
      std::exit(1);
    }

    if (CurrentToken.Type == TokenType::RightBrace)
    {
      break;
    }

    block.push_back(ParseStatement());
  }

  spn.RangeEnd = CurrentToken.Spn.RangeEnd;
  Bump(); // eat '}'

  return BlockStatement(spn, block);
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
    Diagnostic.Error(ErrorCode::UnexpectedToken, "Unexpected expression.", CurrentToken.Spn);
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
  Span spn                = callee->Spn;
  spn.RangeEnd            = args.Spn.RangeEnd;
  return new CallExpression(spn, callee, args);
}

CallExpressionArgs Parser::ParseCallExpressionArgs()
{
  Span spn = CurrentToken.Spn;
  Bump();

  // case 1: ()
  if (TokenType::RightParent == CurrentToken.Type)
  {
    spn.RangeEnd = CurrentToken.Spn.RangeEnd;
    Bump();
    return CallExpressionArgs(spn, {});
  }

  std::vector<Expression *> args;

  // case 2: (x)
  args.push_back(ParseExpressionStatement(Precedence::Lowest));

  // case 3: (x, y, ...)
  while (TokenType::RightParent != CurrentToken.Type)
  {
    if (TokenType::Eof == CurrentToken.Type)
    {
      Diagnostic.Error(ErrorCode::UnexpectedToken, "Unexpected EOF.", CurrentToken.Spn);
      std::exit(1);
    }

    if (TokenType::Comma != CurrentToken.Type)
    {
      Diagnostic.Error(ErrorCode::UnexpectedToken, "Expect `)` or `,` to separate function's arguments.",
                       CurrentToken.Spn);
      std::exit(1);
    }

    Bump(); // eat `,`
    args.push_back(ParseExpressionStatement(Precedence::Lowest));
  }

  spn.RangeEnd = CurrentToken.Spn.RangeEnd;
  Bump();

  return CallExpressionArgs(spn, args);
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
    Diagnostic.Error(ErrorCode::UnexpectedToken, "Expect ':' before type specifier.", CurrentToken.Spn);
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

  Diagnostic.Error(ErrorCode::UnexpectedToken, "Invalid type annotation", CurrentToken.Spn);
  std::exit(1);
}
