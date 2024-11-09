#pragma once

#include "DiagnosticEngine.h"
#include "Token.h"

#include <filesystem>

class Lexer
{
public:
  const std::filesystem::path FilePath;
  const std::string &FileContent;

  Lexer(std::filesystem::path filePath, std::string &fileContent, DiagnosticEngine &diagnostic)
      : FilePath(filePath), FileContent(fileContent), Diagnostic(diagnostic)
  {
    Cursor     = 0;
    Line       = 1;
    Column     = 1;
    RangeBegin = 0;
  };

  Token GetNextToken();

private:
  DiagnosticEngine &Diagnostic;

  unsigned long Cursor;
  unsigned long Line;
  unsigned long Column;
  unsigned long RangeBegin;

  bool IsEof();
  void SkipWhitespace();
  char PeekOne();
  void AdvanceOne();
  void UpdateTokenSpan();

  Span MakeTokenSpan();
  Token MakeStringToken();
  Token MakeSimpleToken(TokenType);
};
