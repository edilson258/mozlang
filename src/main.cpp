#include <cstdio>
#include <iostream>
#include <memory>
#include <string>

#include "checker.h"
#include "diagnostic.h"
#include "ir/ir.h"
#include "lexer.h"
#include "loader.h"
#include "parser.h"

int main(int argc, char *argv[])
{
  if (argc < 2)
  {
    std::cerr << "Usage: " << argv[0] << " <input_file>" << std::endl;
    return 1;
  }
  std::string entryFile = argv[1];
  Loader loader;
  auto entryRes = loader.Load(entryFile);
  if (entryRes.is_err())
  {
    std::cerr << entryRes.unwrap_err().Message << std::endl;
    return 1;
  }
  auto entrySource = entryRes.unwrap();
  Lexer lexer(entrySource);
  Parser parser(lexer);
  auto astRes = parser.Parse();
  if (astRes.is_err())
  {
    DiagnosticEngine::Report(astRes.unwrap_err());
    return 1;
  }
  AST ast = astRes.unwrap();
  std::cout << ast.Inspect() << std::endl;
  Checker checker(ast, entrySource);
  auto diagnostics = checker.check();
  bool hasErrorDiagnostic = false;
  for (auto &diagnostic : diagnostics)
  {
    if (DiagnosticSeverity::ERROR == diagnostic.m_Severity)
    {
      hasErrorDiagnostic = true;
    }
    DiagnosticEngine::Report(diagnostic);
  }
  if (hasErrorDiagnostic)
  {
    return 1;
  }
  IRGenerator irGenerator(ast);
  auto irRes = irGenerator.Emit();
  auto ir = irRes.unwrap();
  IRDisassembler irDesassembler(ir);
  auto irAsm = irDesassembler.Disassemble();
  std::cout << irAsm.unwrap() << std::endl;
}
