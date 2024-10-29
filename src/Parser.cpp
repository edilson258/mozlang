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
    NextToken = lexer.GetNextToken();
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

std::unique_ptr<Statement> Parser::ParseStatement()
{
    if (CurrentToken.Type == TokenType::Fn)
    {
        return ParseFunctionStatement();
    }

    auto expression = ParseExpressionStatement(Precedence::Lowest);
    BumpExpected(TokenType::Semicolon);
    return expression;
}

std::unique_ptr<FunctionStatement> Parser::ParseFunctionStatement()
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

    return std::make_unique<FunctionStatement>(identifier, std::move(body));
}

BlockStatement Parser::ParseBlockStatement()
{
    Bump(); // eat '{'

    std::vector<std::unique_ptr<Statement>> block;

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

    return BlockStatement(std::move(block));
}

std::unique_ptr<Expression> Parser::ParseExpressionStatement(Precedence precedence)
{
    std::unique_ptr<Expression> leftHandSide;

    if (CurrentToken.Type == TokenType::Identifier)
    {
        leftHandSide = std::make_unique<IdentifierExpression>(CurrentToken);
    }
    else if (CurrentToken.Type == TokenType::String)
    {
        leftHandSide = std::make_unique<StringExpression>(CurrentToken);
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

std::unique_ptr<CallExpression> Parser::ParseCallExpression(std::unique_ptr<Expression> callee)
{
    Bump(); // eat '('

    // TODO: parse args
    std::vector<std::unique_ptr<Expression>> args;
    args.push_back(ParseExpressionStatement(Precedence::Call));

    BumpExpected(TokenType::RightParent);

    return std::make_unique<CallExpression>(std::move(callee), std::move(args));
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