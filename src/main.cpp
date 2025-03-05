#include <cstdio>
#include <iostream>
#include <memory>
#include <string>

#include "checker.h"
#include "context.h"
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

  ModuleManager moduleManager;
  DiagnosticEngine diagnosticEngine(moduleManager);
  ModuleID entryModID = moduleManager.Load(argv[1]).unwrap();

  Lexer lexer(entryModID, moduleManager);
  Parser parser(entryModID, lexer);
  auto astRes = parser.Parse();
  if (astRes.is_err())
  {
    diagnosticEngine.Report(astRes.unwrap_err());
    return 1;
  }
  AST ast = astRes.unwrap();
  std::cout << ast.Inspect() << std::endl;
  auto outContext = std::make_shared<Context>(Context());
  Checker checker(ast, moduleManager, outContext);
  auto diagnostics = checker.Check();
  bool hasErrorDiagnostic = false;
  for (auto &diagnostic : diagnostics)
  {
    if (DiagnosticSeverity::ERROR == diagnostic.m_Severity)
    {
      hasErrorDiagnostic = true;
    }
    diagnosticEngine.Report(diagnostic);
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
