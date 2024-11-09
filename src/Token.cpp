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
  case TokenType::Return:
    oss << "Keyword: " << Data;
    break;
  case TokenType::String:
    oss << "String: \"" << Data << "\"";
    break;
  case TokenType::Integer:
    oss << "Integer: " << Data;
    break;
  case TokenType::Identifier:
    oss << "Ident.: " << Data;
    break;
  case TokenType::LeftParent:
  case TokenType::RightParent:
  case TokenType::LeftBrace:
  case TokenType::RightBrace:
  case TokenType::Semicolon:
  case TokenType::Colon:
  case TokenType::Comma:
    oss << "Punct.: " << Data;
    break;
  case TokenType::TypeInt:
  case TokenType::TypeStr:
  case TokenType::TypeVoid:
    oss << "Type: " << Data;
    break;
  }

  oss << " " << Spn.Line << ":" << Spn.Column << ":" << Spn.RangeBegin << ":" << Spn.RangeEnd << std::endl;

  return oss.str();
}
