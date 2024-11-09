#include "Lexer.h"
#include "DiagnosticEngine.h"
#include "Token.h"

#include <cctype>
#include <cstdlib>
#include <format>
#include <string>

bool Lexer::IsEof() { return FileContent.length() <= Cursor; }

void Lexer::SkipWhitespace()
{
  while (!IsEof() && std::isspace(PeekOne()))
  {
    AdvanceOne();
  }
}

char Lexer::PeekOne()
{
  if (IsEof())
  {
    return 0;
  }

  return FileContent.at(Cursor);
}

void Lexer::AdvanceOne()
{
  if (IsEof())
  {
    return;
  }

  Cursor++;

  if (PeekOne() == '\n')
  {
    Line++;

    // why not 1? -> I set 0 to avoid counting the '\n'
    Column = 0;
  }
  else
  {
    Column++;
  }
}

void Lexer::UpdateTokenSpan() { RangeBegin = Cursor; }

Span Lexer::MakeTokenSpan() { return Span(Line, Column, RangeBegin, Cursor); }

Token Lexer::MakeSimpleToken(TokenType type)
{
  Token simpleToken(type, MakeTokenSpan(), std::string(1, PeekOne()));
  AdvanceOne();
  return simpleToken;
}

Token Lexer::MakeStringToken()
{
  // column where string begins including left quote
  unsigned long stringBeginColumn = Column;

  AdvanceOne(); // eat left '"'

  // Absolute index where string begins, NOT icluding left quote
  unsigned long stringBeginIndex = Cursor;

  while (1)
  {
    if (IsEof() || PeekOne() == '\n')
    {
      Diagnostic.Error(ErrorCode::UnquotedString, "Unquoted string literal",
                       Span(Line - 1, stringBeginColumn, stringBeginIndex - 1, Cursor));
      std::exit(1);
    }

    if (PeekOne() == '"')
    {
      break;
    }

    AdvanceOne();
  }

  AdvanceOne(); // eat right '"'

  Span stringSpan = Span(Line, stringBeginColumn, RangeBegin, Cursor - 1);
  return Token(TokenType::String, stringSpan, FileContent.substr(stringBeginIndex, Cursor - stringBeginIndex - 1));
}

Token Lexer::GetNextToken()
{
  SkipWhitespace();
  UpdateTokenSpan();

  if (IsEof())
  {
    return MakeSimpleToken(TokenType::Eof);
  }

  char currentChar = PeekOne();

  switch (currentChar)
  {
  case '(':
    return MakeSimpleToken(TokenType::LeftParent);
  case ')':
    return MakeSimpleToken(TokenType::RightParent);
  case '{':
    return MakeSimpleToken(TokenType::LeftBrace);
  case '}':
    return MakeSimpleToken(TokenType::RightBrace);
  case ';':
    return MakeSimpleToken(TokenType::Semicolon);
  case ':':
    return MakeSimpleToken(TokenType::Colon);
  case ',':
    return MakeSimpleToken(TokenType::Comma);
  case '"':
    return MakeStringToken();
  }

  if (std::isalpha(currentChar) || '_' == currentChar)
  {
    unsigned long identifierBeginIndex  = Cursor;
    unsigned long identifierBeginColumn = Column;

    while (!IsEof() && (std::isalnum(PeekOne()) || '_' == PeekOne()))
    {
      AdvanceOne();
    }

    Span identifierSpan         = Span(Line, identifierBeginColumn, RangeBegin, Cursor - 1);
    std::string identifierLabel = FileContent.substr(identifierBeginIndex, Cursor - identifierBeginIndex);

    if ("fn" == identifierLabel)
    {
      return Token(TokenType::Fn, identifierSpan, identifierLabel);
    }

    if ("return" == identifierLabel)
    {
      return Token(TokenType::Return, identifierSpan, identifierLabel);
    }

    if ("int" == identifierLabel)
    {
      return Token(TokenType::TypeInt, identifierSpan, identifierLabel);
    }

    if ("str" == identifierLabel)
    {
      return Token(TokenType::TypeStr, identifierSpan, identifierLabel);
    }

    if ("void" == identifierLabel)
    {
      return Token(TokenType::TypeVoid, identifierSpan, identifierLabel);
    }

    return Token(TokenType::Identifier, identifierSpan, identifierLabel);
  }

  if (std::isdigit(currentChar))
  {
    unsigned long intBeginIndex  = Cursor;
    unsigned long intBeginColumn = Column;

    while (!IsEof() && std::isdigit(PeekOne()))
    {
      AdvanceOne();
    }

    Span intSpan       = Span(Line, intBeginColumn, RangeBegin, Cursor - 1);
    std::string intRaw = FileContent.substr(intBeginIndex, Cursor - intBeginIndex);
    return Token(TokenType::Integer, intSpan, intRaw);
  }

  Diagnostic.Error(ErrorCode::UnknownToken, std::format("Unknown token '{}'", currentChar),
                   Span(Line, Column, Cursor, Cursor));
  std::exit(1);
}
