#include "Lexer.h"
#include "Parser.h"
#include "llvm/IR/IRBuilder.h"

#include <iostream>

#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>

int main()
{
  std::string code = "fn main() {\n"
                     "  print(\"Hello Mozambique!\");\n"
                     "}";

  Lexer lexer(code);
  Parser parser(lexer);

  AST ast = parser.Parse();

  std::cout << ast.ToStrng() << std::endl;

  llvm::LLVMContext context;
  llvm::Module *module = new llvm::Module("hello", context);
}