#pragma once

#include <llvm/IR/Module.h>

class ExecutableBuilder
{
public:
  static void Build(llvm::Module *module);
};
