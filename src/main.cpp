#include <cstdio>
#include <iostream>
#include <memory>
#include <string>

#include "checker.h"
#include "diagnostic.h"
#include "ir/ir.h"
#include "module.h"
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
  auto loadRes = moduleManager.Load(argv[1]);
  if (loadRes.is_err())
  {
    std::cerr << loadRes.unwrap_err().Message << std::endl;
  }
  auto mainModule = loadRes.unwrap();
  Parser parser(mainModule, moduleManager);
  auto parseError = parser.Parse();
  if (parseError.has_value())
  {
    diagnosticEngine.Report(parseError.value());
    return 1;
  }
  // std::cout << ast.Inspect() << std::endl;
  Checker checker(mainModule, moduleManager);
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
  IRGenerator irGenerator(mainModule);
  auto irRes = irGenerator.Emit();
  auto ir = irRes.unwrap();
  IRDisassembler irDesassembler(ir);
  auto irAsm = irDesassembler.Disassemble();
  std::cout << irAsm.unwrap() << std::endl;
}
