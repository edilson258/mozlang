#include <cassert>
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <format>
#include <string>

#include "diagnostic.h"
#include "error.h"
#include "keywords.h"
#include "lexer.h"
#include "result.h"
#include "token.h"

#define EOF_CHAR '\0'

Result<Token, Diagnostic> Lexer::Next()
{
  AdvanceWhile([](char c)
               { return std::isspace(c); });

  if (IsEof())
  {
    return Result<Token, Diagnostic>(Token(Position(m_Line, m_Column, m_Cursor, m_Cursor), TokenType::EOf, "EOF"));
  }

  char current = PeekOne();
  switch (current)
  {
  case '(':
    return MakeTokenSimple(TokenType::LPAREN);
  case ')':
    return MakeTokenSimple(TokenType::RPAREN);
  case '{':
    return MakeTokenSimple(TokenType::LBRACE);
  case '}':
    return MakeTokenSimple(TokenType::RBRACE);
  case ':':
    return MakeTokenSimple(TokenType::COLON);
  case ';':
    return MakeTokenSimple(TokenType::SEMICOLON);
  case ',':
    return MakeTokenSimple(TokenType::COMMA);
  case '"':
    return MakeTokenString();
  }

  if (std::isalpha(current) || '_' == current)
  {
    size_t at = m_Cursor;
    size_t atColumn = m_Column;
    size_t len = AdvanceWhile([](char c)
                              { return std::isalnum(c) || '_' == c; });
    std::string label = m_Source.get()->m_Content.substr(at, len);
    std::optional<TokenType> keyword = Keyword::match(label);
    if (keyword.has_value())
    {
      return Result<Token, Diagnostic>(Token(Position(m_Line, atColumn, at, m_Cursor - 1), keyword.value(), label));
    }
    return Result<Token, Diagnostic>(Token(Position(m_Line, atColumn, at, m_Cursor - 1), TokenType::IDENTIFIER, label));
  }

  return Result<Token, Diagnostic>(Diagnostic(Errno::SYNTAX_ERROR, Position(m_Line, m_Column, m_Cursor, m_Cursor), m_Source, DiagnosticSeverity::ERROR, std::format("invalid token: '{}'", current)));
}

Result<Token, Diagnostic> Lexer::MakeTokenSimple(TokenType tt)
{
  Token token(Position(m_Line, m_Column, m_Cursor, m_Cursor), tt, std::string(1, PeekOne()));
  Advance();
  return Result<Token, Diagnostic>(token);
}

Result<Token, Diagnostic> Lexer::MakeTokenString()
{
  size_t at = m_Cursor;
  size_t atColumn = m_Column;

  Advance();
  for (;;)
  {
    char current = PeekOne();
    if (IsEof() || '\n' == current)
    {
      return Result<Token, Diagnostic>(Diagnostic(Errno::SYNTAX_ERROR, Position(m_Line, m_Column, at, m_Cursor - 1), m_Source, DiagnosticSeverity::ERROR, "unquoted string"));
    }
    if ('"' == current)
    {
      Advance();
      break;
    }
    Advance();
  }

  return Result<Token, Diagnostic>(Token(Position(m_Line, atColumn, at, m_Cursor - 1), TokenType::STRING, m_Source.get()->m_Content.substr(at, m_Cursor - at)));
}

bool Lexer::IsEof()
{
  return m_Cursor >= m_Source.get()->m_Content.length();
}

char Lexer::PeekOne()
{
  if (IsEof())
  {
    return EOF_CHAR;
  }
  return m_Source.get()->m_Content.at(m_Cursor);
}

void Lexer::Advance()
{
  if (IsEof())
  {
    return;
  }
  m_Cursor++;
  if ('\n' == PeekOne())
  {
    m_Line++;
    m_Column = 0;
  }
  else
  {
    m_Column++;
  }
}

size_t Lexer::AdvanceWhile(std::function<bool(char)> predicate)
{
  size_t at = m_Cursor;
  while (!IsEof() && predicate(PeekOne()))
  {
    Advance();
  }
  return m_Cursor - at;
}
