#pragma once

#include "Token.h"

#include <filesystem>

class Lexer
{
public:
  const std::string &FileContent;
  const std::filesystem::path FilePath;

  Lexer(std::string &fileContent, std::filesystem::path filePath) : FileContent(fileContent), FilePath(filePath)
  {
    Cursor     = 0;
    Line       = 1;
    Column     = 1;
    RangeBegin = 0;
  };

  Token GetNextToken();

private:
  unsigned long Cursor;
  unsigned long Line;
  unsigned long Column;
  unsigned long RangeBegin;

  bool IsEof();
  void SkipWhitespace();
  char PeekOne();
  void AdvanceOne();
  void UpdateTokenSpan();

  TokenSpan MakeTokenSpan();
  Token MakeStringToken();
  Token MakeSimpleToken(TokenType);
};
