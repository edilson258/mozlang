#pragma once

#include "ast.h"
#include "diagnostic.h"
#include "lexer.h"

class Parser
{
public:
  Parser(Lexer &lexer) : m_Lexer(lexer), m_CurrToken(), m_NextToken() {};

  Result<AST, Diagnostic> Parse();

private:
  Lexer &m_Lexer;
  Token m_CurrToken;
  Token m_NextToken;

  bool IsEof();
  Result<Position, Diagnostic> Next();
  Result<Position, Diagnostic> Expect(TokenType);

  Result<std::shared_ptr<Statement>, Diagnostic> ParseStatement();
  Result<std::shared_ptr<StatementFunction>, Diagnostic> ParseStatementFunction();
  Result<std::shared_ptr<StatementReturn>, Diagnostic> ParseStatementReturn();
  Result<std::shared_ptr<StatementBlock>, Diagnostic> ParseStatementBlock();
  Result<std::shared_ptr<Expression>, Diagnostic> ParseExpression(Precedence);
  Result<std::shared_ptr<Expression>, Diagnostic> ParseExpressionLhs();
  Result<std::shared_ptr<ExpressionCall>, Diagnostic> ParseExpressionCall(std::shared_ptr<Expression>);

  Result<FunctionParams, Diagnostic> ParseFunctionParams();
};
