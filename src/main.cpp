#include <iostream>
#include <memory>
#include <string>

#include "lexer.h"
#include "loader.h"
#include "parser.h"

int main(void)
{
  std::string zero_code = "println(\"{}\", \"Hello, world!\");";
  auto src = std::make_shared<source>(1, "./main.zr", zero_code);
  lexer l(src);
  parser p(l);
  auto ast = p.parse();
  std::cout << ast.unwrap().get()->inspect();
}
