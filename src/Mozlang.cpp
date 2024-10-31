#include "Lexer.h"
#include "Parser.h"

#include <iostream>

int main()
{
  std::string code = "fn main() {\n"
                     "  print(\"Hello Mozambique!\");\n"
                     "}";

  Lexer lexer(code);
  Parser parser(lexer);

  AST ast = parser.Parse();

  std::cout << ast.ToStrng() << std::endl;
}