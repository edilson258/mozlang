#pragma once

#include "ast.h"
#include "diagnostic.h"
#include "lexer.h"
#include "loader.h"
#include "result.h"

class Parser
{
public:
  Parser(ModuleID moduleID, Lexer &lexer) : m_ModuleID(moduleID), m_Lexer(lexer), m_CurrToken(), m_NextToken() {};

  Result<AST, Diagnostic> Parse();

private:
  ModuleID m_ModuleID;
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
  Result<std::shared_ptr<StatementLet>, Diagnostic> ParseStatementLet();
  Result<std::shared_ptr<Expression>, Diagnostic> ParseExpression(Precedence);
  Result<std::shared_ptr<Expression>, Diagnostic> ParseExpressionPrimary();
  Result<std::shared_ptr<ExpressionIdentifier>, Diagnostic> ParseExpressionIdentifier();
  Result<std::shared_ptr<ExpressionCall>, Diagnostic> ParseExpressionCall(std::shared_ptr<Expression>);
  Result<std::shared_ptr<ExpressionAssign>, Diagnostic> ParseExpressionAssign(std::shared_ptr<Expression>);

  Result<FunctionParams, Diagnostic> ParseFunctionParams();
  Result<std::shared_ptr<type::Type>, Diagnostic> ParseTypeAnnotation();
};
