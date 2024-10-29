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
	case TokenType::String:
		oss << "String: \"" << std::get<std::string>(Data) << "\"";
		break;
	case TokenType::Identifier:
		oss << "Ident.: " << std::get<std::string>(Data);
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
	}

	oss << " " << Span.Line << ":" << Span.Column << ":" << Span.RangeBegin << ":" << Span.RangeEnd << std::endl;

	return oss.str();
}