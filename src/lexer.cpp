#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstddef>
#include <cstdlib>
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
    return Token(Position(m_Line, m_Column, m_Cursor, m_Cursor), TokenType::END, "EOF");
  }
  char current = PeekOne();
  // {+-}[0-9]
  if (std::isdigit(current) || ((current == '-' || current == '+') && std::isdigit(PeekNext())))
  {
    return MakeTokenNumber();
  }
  if (std::isalpha(current) || '_' == current)
  {
    size_t at = m_Cursor;
    size_t atColumn = m_Column;
    size_t len = AdvanceWhile([](char c)
                              { return std::isalnum(c) || '_' == c; });
    std::string label = m_ModuleContent.substr(at, len);
    std::optional<TokenType> keyword = Keyword::match(label);
    if (keyword.has_value())
    {
      return Token(Position(m_Line, atColumn, at, m_Cursor - 1), keyword.value(), label);
    }
    return Token(Position(m_Line, atColumn, at, m_Cursor - 1), TokenType::Ident, label);
  }
  switch (current)
  {
  case '@':
    return MakeTokenSimple(TokenType::At);
  case '(':
    return MakeTokenSimple(TokenType::Lparen);
  case ')':
    return MakeTokenSimple(TokenType::Rparen);
  case '{':
    return MakeTokenSimple(TokenType::Lbrace);
  case '}':
    return MakeTokenSimple(TokenType::Rbrace);
  case ';':
    return MakeTokenSimple(TokenType::Semi);
  case '=':
    return MakeTokenSimple(TokenType::Equal);
  case ',':
    return MakeTokenSimple(TokenType::Comma);
  case ':':
    return MakeIfNextOr(":", TokenType::Assoc, TokenType::Colon);
  case '.':
    return MakeIfNextOr("..", TokenType::Ellipsis, TokenType::Dot);
  case '-':
    return MakeIfNextOr(">", TokenType::Arrow, TokenType::Minus);
  case '"':
    return MakeTokenString();
  }
  std::string message = "Unexpected token: ";
  message.push_back(current);
  return Diagnostic(Errno::SYNTAX_ERROR, Position(m_Line, m_Column, m_Cursor, m_Cursor), m_ModuleID, DiagnosticSeverity::ERROR, message);
}

Result<Token, Diagnostic> Lexer::MakeTokenSimple(TokenType tt)
{
  Token token(Position(m_Line, m_Column, m_Cursor, m_Cursor), tt, std::string(1, PeekOne()));
  Advance();
  return token;
}

Result<Token, Diagnostic> Lexer::MakeIfNextOr(std::string next, TokenType tt1, TokenType tt2)
{
  Token token(Position(m_Line, m_Column, m_Cursor, m_Cursor), tt2, std::string(1, PeekOne()));
  Advance();
  if (StartsWith(next))
  {
    for (size_t i = 0; i < next.length(); ++i)
    {
      Advance();
    }
    token.m_Type = tt1;
    token.m_Position.m_End = m_Cursor;
    token.m_Lexeme.append(next);
  }
  return token;
}

Result<Token, Diagnostic> Lexer::MakeTokenString()
{
  size_t atColumn = m_Column;
  Advance();
  size_t at = m_Cursor;
  for (;;)
  {
    char current = PeekOne();
    if (IsEof() || '\n' == current)
    {
      return Diagnostic(Errno::SYNTAX_ERROR, Position(m_Line, m_Column, at, m_Cursor - 1), m_ModuleID, DiagnosticSeverity::ERROR, "unquoted string");
    }
    if ('"' == current)
    {
      break;
    }
    Advance();
  }
  size_t len = m_Cursor - at;
  Advance();
  return Token(Position(m_Line, atColumn, at - 1, m_Cursor - 1), TokenType::StrLit, m_ModuleContent.substr(at, len));
}

Result<Token, Diagnostic> Lexer::MakeTokenNumber()
{
  auto at = m_Cursor;
  auto atCol = m_Column;
  size_t len = 0;
  char current = PeekOne();
  if (current == '+' || current == '-')
  {
    len++;
    Advance();
  }
  if (StartsWith("0b"))
  {
    len += 2;
    Advance(2);
    auto bufLen = AdvanceWhile([](char c)
                               { return c == '0' || c == '1'; });
    assert(bufLen > 0);
    len += bufLen;
    return Token(Position(m_Line, atCol, at, m_Cursor - 1), TokenType::BinLit, m_ModuleContent.substr(at, len));
  }
  else if (StartsWith("0x"))
  {
    len += 2;
    Advance(2);
    size_t bufLen = AdvanceWhile([](char c)
                                 { return std::isdigit(c) || ((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')); });
    assert(bufLen > 0);
    len += bufLen;
    return Token(Position(m_Line, atCol, at, m_Cursor - 1), TokenType::HexLit, m_ModuleContent.substr(at, len));
  }
  len += AdvanceWhile([](char c)
                      { return std::isdigit(c) || c == '.'; });
  std::string label = m_ModuleContent.substr(at, len);
  // TODO: validate number
  return Token(Position(m_Line, atCol, at, m_Cursor - 1), std::find(label.begin(), label.end(), '.') == label.end() ? TokenType::DecLit : TokenType::FloatLit, label);
}

bool Lexer::IsEof()
{
  return m_Cursor >= m_ModuleContent.length();
}

char Lexer::PeekOne()
{
  if (IsEof())
  {
    return EOF_CHAR;
  }
  return m_ModuleContent.at(m_Cursor);
}

char Lexer::PeekNext()
{
  if (m_Cursor + 1 >= m_ModuleContent.length())
  {
    return EOF_CHAR;
  }
  return m_ModuleContent.at(m_Cursor + 1);
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

void Lexer::Advance(size_t steps)
{
  for (size_t i = 0; i < steps; ++i)
  {
    Advance();
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

bool Lexer::StartsWith(std::string xs)
{
  return xs == m_ModuleContent.substr(m_Cursor, xs.length());
}
