#pragma once

#include <cstddef>
#include <functional>
#include <memory>

#include "diagnostic.h"
#include "loader.h"
#include "result.h"
#include "token.h"

class Lexer
{
public:
  std::shared_ptr<Source> Sourc;

  Lexer(std::shared_ptr<Source> source) : Sourc(source), Line(1), Column(1), Cursor(0) {};

  result<Token, Diagnostic> Next();

private:
  size_t Line;
  size_t Column;

  size_t Cursor;

  bool IsEof();
  char PeekOne();
  void Advance();
  size_t AdvanceWhile(std::function<bool(char)>);

  result<Token, Diagnostic> MakeTokenString();
  result<Token, Diagnostic> MakeTokenSimple(TokenType);
};
