#pragma once

#include "Token.h"

class Lexer
{
public:
  const std::string &FileContent;

  Lexer(std::string &fileContent) : FileContent(fileContent)
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