#include <cassert>
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <format>
#include <string>

#include "diagnostic.h"
#include "error.h"
#include "lexer.h"
#include "result.h"
#include "token.h"

#define EOF_CHAR '\0'

result<Token, Diagnostic> Lexer::Next()
{
  AdvanceWhile([](char c)
               { return std::isspace(c); });

  if (IsEof())
  {
    return result<Token, Diagnostic>(Token(Position(Line, Column, Cursor, Cursor), TokenType::EOf, "EOF"));
  }

  char current = PeekOne();
  switch (current)
  {
  case '(':
    return MakeTokenSimple(TokenType::LPAREN);
  case ')':
    return MakeTokenSimple(TokenType::RPAREN);
  case ';':
    return MakeTokenSimple(TokenType::SEMICOLON);
  case ',':
    return MakeTokenSimple(TokenType::COMMA);
  case '"':
    return MakeTokenString();
  }

  if (std::isalpha(current) || '_' == current)
  {
    size_t at = Cursor;
    size_t atColumn = Column;
    size_t len = AdvanceWhile([](char c)
                              { return std::isalnum(c) || '_' == c; });
    return result<Token, Diagnostic>(Token(Position(Line, atColumn, at, Cursor - 1), TokenType::IDENTIFIER, Sourc.get()->content.substr(at, len)));
  }

  return result<Token, Diagnostic>(Diagnostic(Errno::SYNTAX_ERROR, Position(Line, Column, Cursor, Cursor), Sourc, DiagnosticSeverity::ERROR, std::format("invalid token: '{}'", current)));
}

result<Token, Diagnostic> Lexer::MakeTokenSimple(TokenType tt)
{
  Token token(Position(Line, Column, Cursor, Cursor), tt, std::string(1, PeekOne()));
  Advance();
  return result<Token, Diagnostic>(token);
}

result<Token, Diagnostic> Lexer::MakeTokenString()
{
  size_t at = Cursor;
  size_t atColumn = Column;

  Advance();
  for (;;)
  {
    char current = PeekOne();
    if (IsEof() || '\n' == current)
    {
      return result<Token, Diagnostic>(Diagnostic(Errno::SYNTAX_ERROR, Position(Line, Column, at, Cursor - 1), Sourc, DiagnosticSeverity::ERROR, "unquoted string"));
    }
    if ('"' == current)
    {
      Advance();
      break;
    }
    Advance();
  }

  return result<Token, Diagnostic>(Token(Position(Line, atColumn, at, Cursor - 1), TokenType::STRING, Sourc.get()->content.substr(at, Cursor - at)));
}

bool Lexer::IsEof()
{
  return Cursor >= Sourc.get()->content.length();
}

char Lexer::PeekOne()
{
  if (IsEof())
  {
    return EOF_CHAR;
  }
  return Sourc.get()->content.at(Cursor);
}

void Lexer::Advance()
{
  if (IsEof())
  {
    return;
  }
  Cursor++;
  if ('\n' == PeekOne())
  {
    Line++;
    Column = 0;
  }
  else
  {
    Column++;
  }
}

size_t Lexer::AdvanceWhile(std::function<bool(char)> predicate)
{
  size_t at = Cursor;
  while (!IsEof() && predicate(PeekOne()))
  {
    Advance();
  }
  return Cursor - at;
}
