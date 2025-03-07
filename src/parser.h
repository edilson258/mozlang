#pragma once

#include <memory>
#include <optional>

#include "ast.h"
#include "diagnostic.h"
#include "lexer.h"
#include "module.h"
#include "result.h"

class Parser
{
public:
  Parser(std::shared_ptr<Module> module, ModuleManager &modManager) : m_Module(module), m_ModuleID(module.get()->m_ID), m_Lexer(Lexer(module.get()->m_ID, modManager)), m_CurrToken(), m_NextToken() {};

  std::optional<Diagnostic> Parse();

private:
  std::shared_ptr<Module> m_Module;
  ModuleID m_ModuleID;
  Lexer m_Lexer;
  Token m_CurrToken;
  Token m_NextToken;

  bool IsEof();
  Result<Position, Diagnostic> Next();
  Result<Position, Diagnostic> Expect(TokenType);

  Result<std::shared_ptr<Statement>, Diagnostic> ParseStatement();
  Result<std::shared_ptr<Statement>, Diagnostic> ParseStatementFunction();
  Result<std::shared_ptr<Statement>, Diagnostic> ParseStatementReturn();
  Result<std::shared_ptr<Statement>, Diagnostic> ParseStatementLet();
  Result<std::shared_ptr<Statement>, Diagnostic> ParseStatementImport();
  Result<std::shared_ptr<Statement>, Diagnostic> ParseStatementBlock();
  Result<std::shared_ptr<Expression>, Diagnostic> ParseExpression(Precedence);
  Result<std::shared_ptr<Expression>, Diagnostic> ParseExpressionPrimary();
  Result<std::shared_ptr<ExpressionIdentifier>, Diagnostic> ParseExpressionIdentifier();
  Result<std::shared_ptr<ExpressionCall>, Diagnostic> ParseExpressionCall(std::shared_ptr<Expression>);
  Result<std::shared_ptr<ExpressionAssign>, Diagnostic> ParseExpressionAssign(std::shared_ptr<Expression>);
  Result<std::shared_ptr<ExpressionFieldAccess>, Diagnostic> ParseExpressionFieldAccess(std::shared_ptr<Expression>);

  Result<FunctionParams, Diagnostic> ParseFunctionParams();
  Result<AstType, Diagnostic> ParseTypeAnnotation();
  Result<AstType, Diagnostic> ParseTypeAnnotationFunction();
};
