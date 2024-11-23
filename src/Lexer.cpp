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

void Lexer::UpdateTokenLoc() { RangeBegin = Cursor; }

Location Lexer::MakeTokenLocation() { return Location(Line, Column, RangeBegin, Cursor); }

Token Lexer::MakeSimpleToken(TokenType type)
{
  Token simpleToken(type, MakeTokenLocation(), std::string(1, PeekOne()));
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
                       Location(Line - 1, stringBeginColumn, stringBeginIndex - 1, Cursor));
      std::exit(1);
    }

    if (PeekOne() == '"')
    {
      break;
    }

    AdvanceOne();
  }

  AdvanceOne(); // eat right '"'

  Location stringLoc = Location(Line, stringBeginColumn, RangeBegin, Cursor - 1);
  return Token(TokenType::String, stringLoc, FileContent.substr(stringBeginIndex, Cursor - stringBeginIndex - 1));
}

Token Lexer::GetNextToken()
{
  SkipWhitespace();
  UpdateTokenLoc();

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

    Location identifierLoc      = Location(Line, identifierBeginColumn, RangeBegin, Cursor - 1);
    std::string identifierLabel = FileContent.substr(identifierBeginIndex, Cursor - identifierBeginIndex);

    if ("fn" == identifierLabel)
    {
      return Token(TokenType::Fn, identifierLoc, identifierLabel);
    }

    if ("return" == identifierLabel)
    {
      return Token(TokenType::Return, identifierLoc, identifierLabel);
    }

    if ("int" == identifierLabel)
    {
      return Token(TokenType::TypeInt, identifierLoc, identifierLabel);
    }

    if ("str" == identifierLabel)
    {
      return Token(TokenType::TypeStr, identifierLoc, identifierLabel);
    }

    if ("void" == identifierLabel)
    {
      return Token(TokenType::TypeVoid, identifierLoc, identifierLabel);
    }

    if ("bool" == identifierLabel)
    {
      return Token(TokenType::TypeBool, identifierLoc, identifierLabel);
    }

    if ("true" == identifierLabel)
    {
      return Token(TokenType::True, identifierLoc, identifierLabel);
    }

    if ("false" == identifierLabel)
    {
      return Token(TokenType::False, identifierLoc, identifierLabel);
    }

    return Token(TokenType::Identifier, identifierLoc, identifierLabel);
  }

  if (std::isdigit(currentChar))
  {
    unsigned long intBeginIndex  = Cursor;
    unsigned long intBeginColumn = Column;

    while (!IsEof() && std::isdigit(PeekOne()))
    {
      AdvanceOne();
    }

    Location intLoc    = Location(Line, intBeginColumn, RangeBegin, Cursor - 1);
    std::string intRaw = FileContent.substr(intBeginIndex, Cursor - intBeginIndex);
    return Token(TokenType::Integer, intLoc, intRaw);
  }

  Diagnostic.Error(ErrorCode::UnknownToken, std::format("Unknown token '{}'", currentChar),
                   Location(Line, Column, Cursor, Cursor));
  std::exit(1);
}
