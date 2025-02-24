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
  std::string input_file = argv[1];
  loader ldr;
  auto entry_res = ldr.load(input_file);
  if (entry_res.is_err())
  {
    std::cerr << entry_res.unwrap_err().message << std::endl;
    return 1;
  }
  auto entry_src = entry_res.unwrap();
  lexer l(entry_src);
  parser p(l);
  auto ast = p.parse();
  // std::cout << ast.unwrap().get()->inspect();
  checker ckr(*ast.unwrap().get(), entry_src);
  diagnostic_engine de;
  auto xs = ckr.check();
  bool has_errors = false;
  for (auto &x : xs)
  {
    if (x.severity == diagnostic_severity::error)
    {
      has_errors = true;
    }
    de.report(x);
  }
  if (has_errors)
  {
    return 1;
  }
  ir_generator ig(*ast.unwrap().get());
  auto ir_ = ig.emit();
  ir_disassembler id(ir_.unwrap());
  std::cout << id.disassemble().unwrap() << std::endl;
}
