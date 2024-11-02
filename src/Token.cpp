#include "Token.h"

#include <sstream>

std::string Token::ToString()
{
  std::ostringstream oss;

  switch (Type)
  {
  case TokenType::Eof:
    oss << "EOF: Eof";
    break;
  case TokenType::Fn:
    oss << "Keyword: fn";
    break;
  case TokenType::Return:
    oss << "Keyword: return";
    break;
  case TokenType::String:
    oss << "String: \"" << Data.value() << "\"";
    break;
  case TokenType::Integer:
    oss << "Integer: " << Data.value();
    break;
  case TokenType::Identifier:
    oss << "Ident.: " << Data.value();
    break;
  case TokenType::LeftParent:
    oss << "Punct.: (";
    break;
  case TokenType::RightParent:
    oss << "Punct.: )";
    break;
  case TokenType::LeftBrace:
    oss << "Punct.: {";
    break;
  case TokenType::RightBrace:
    oss << "Punct.: }";
    break;
  case TokenType::Semicolon:
    oss << "Punct.: ;";
    break;
  case TokenType::Colon:
    oss << "Punct.: :";
    break;
  case TokenType::TypeInt:
    oss << "Type: int";
    break;
  }

  oss << " " << Span.Line << ":" << Span.Column << ":" << Span.RangeBegin << ":" << Span.RangeEnd << std::endl;

  return oss.str();
}
