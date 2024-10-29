#include "Lexer.h"

#include <iostream>

bool Lexer::IsEof()
{
	return FileContent.length() <= Cursor;
}

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

void Lexer::UpdateTokenSpan()
{
	RangeBegin = Cursor;
}

TokenSpan Lexer::MakeTokenSpan()
{
	return TokenSpan(Line, Column, RangeBegin, Cursor);
}

Token Lexer::MakeSimpleToken(TokenType type)
{
	Token simpleToken(type, MakeTokenSpan());
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
			std::cerr << "[ERROR]: Unquoted string\n";
			std::exit(1);
		}

		if (PeekOne() == '"')
		{
			break;
		}

		AdvanceOne();
	}

	AdvanceOne(); // eat right '"'

	TokenSpan stringSpan = TokenSpan(Line, stringBeginColumn, RangeBegin, Cursor - 1);
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
	case '"':
		return MakeStringToken();
	}

	if (std::isalpha(currentChar) || '_' == currentChar)
	{
		unsigned long identifierBeginIndex = Cursor;
		unsigned long identifierBeginColumn = Column;

		while (!IsEof() && (std::isalnum(PeekOne()) || '_' == PeekOne()))
		{
			AdvanceOne();
		}

		TokenSpan identifierSpan = TokenSpan(Line, identifierBeginColumn, RangeBegin, Cursor - 1);
		std::string identifierLabel = FileContent.substr(identifierBeginIndex, Cursor - identifierBeginIndex);

		if (identifierLabel == "fn")
		{
			return Token(TokenType::Fn, identifierSpan);
		}

		return Token(TokenType::Identifier, identifierSpan, identifierLabel);
	}

	std::cerr << "[ERROR]: Unknown token: " << currentChar << std::endl;
	std::exit(1);
}