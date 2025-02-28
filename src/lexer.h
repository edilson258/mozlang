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
  std::shared_ptr<Source> m_Source;

  Lexer(std::shared_ptr<Source> source) : m_Source(source), m_Line(1), m_Column(1), m_Cursor(0) {};

  Result<Token, Diagnostic> Next();

private:
  size_t m_Line;
  size_t m_Column;

  size_t m_Cursor;

  bool IsEof();
  char PeekOne();
  void Advance();
  size_t AdvanceWhile(std::function<bool(char)>);

  Result<Token, Diagnostic> MakeTokenString();
  Result<Token, Diagnostic> MakeTokenSimple(TokenType);
};
