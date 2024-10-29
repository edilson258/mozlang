#include "Lexer.h"

#include <iostream>

int main(int argc, char* argv[])
{
	std::string Code = "fn main() {\n"
		"  print(\"Hello Mozambique!\");\n"
		"}";

	Lexer lexer(Code);

	while (1)
	{
		Token token = lexer.GetNextToken();

		if (token.Type == TokenType::Eof)
		{
			break;
		}

		std::cout << token.ToString();
	}
}