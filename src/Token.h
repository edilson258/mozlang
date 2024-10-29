#include <string>
#include <variant>

enum class TokenType
{
	Eof,

	// Keywords
	Fn,

	// Literals
	String,

	// Symbols
	Identifier,

	// Punct.
	LeftParent,
	RightParent,
	LeftBrace,
	RightBrace,
	Semicolon,
};

class TokenSpan
{
public:
	unsigned long Line;
	unsigned long Column;
	unsigned long RangeBegin;
	unsigned long RangeEnd;

	TokenSpan() = default;
	TokenSpan(unsigned long line, unsigned long column, unsigned long begin, unsigned long end)
		: Line(line), Column(column), RangeBegin(begin), RangeEnd(end)
	{}
};

class Token
{
public:
	TokenType Type;
	TokenSpan Span;
	std::variant<std::monostate, std::string> Data;

	std::string ToString();

	Token(TokenType type, TokenSpan span) : Type(type), Span(span) {};
	Token(TokenType type, TokenSpan span, std::string data) : Type(type), Span(span), Data(data) {};
};