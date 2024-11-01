#include "CodeGen.h"
#include "Lexer.h"
#include "Parser.h"

#include <filesystem>
#include <fstream>
#include <iostream>

#include <llvm-18/llvm/IR/Module.h>
#include <llvm-18/llvm/Support/raw_ostream.h>

std::string readFile(std::filesystem::path path);

int main(int argc, char *argv[])
{
  if (argc < 2)
  {
    std::cerr << "[ERROR] No input file provided\n";
    return 1;
  }

  std::string code = readFile(argv[1]);

  Lexer lexer(code);
  Parser parser(lexer);

  AST ast = parser.Parse();

  CodeGen gen;
  llvm::Module *module = gen.Generate(ast);

  llvm::outs() << *module;

  return 0;
}

std::string readFile(std::filesystem::path path)
{
  std::ifstream file(path);

  if (!file.is_open())
  {
    std::cerr << "Coudn't open source file" << std::endl;
    abort();
  }

  std::ostringstream oss;
  oss << file.rdbuf();
  return oss.str();
}
