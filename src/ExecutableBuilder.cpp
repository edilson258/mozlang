#include "ExecutableBuilder.h"

#include <cstdlib>
#include <filesystem>
#include <string>

#include <llvm/ADT/APFloat.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>

void ExecutableBuilder::Build(llvm::Module *module)
{
  std::filesystem::path ojectPath = module->getName().str() + ".o";

  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();

  std::string TargetTriple = llvm::sys::getDefaultTargetTriple();
  module->setTargetTriple(TargetTriple);

  std::string Error;
  auto Target = llvm::TargetRegistry::lookupTarget(TargetTriple, Error);
  if (!Target)
  {
    llvm::errs() << "[ERROR] Unknown target: " << Error;
    std::exit(1);
  }

  std::string CPU      = "generic";
  std::string Features = "";
  llvm::TargetOptions opt;
  auto TheTargetMachine = Target->createTargetMachine(TargetTriple, CPU, Features, opt, llvm::Reloc::PIC_);
  module->setDataLayout(TheTargetMachine->createDataLayout());

  std::error_code EC;
  llvm::raw_fd_ostream dest(ojectPath.string(), EC, llvm::sys::fs::OF_None);
  if (EC)
  {
    llvm::errs() << "Could not open object file: " << EC.message();
    std::exit(1);
  }

  llvm::legacy::PassManager pass;
  auto FileType = llvm::CodeGenFileType::ObjectFile;
  if (TheTargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType))
  {
    llvm::errs() << "The target machine can't emit the object file";
    std::exit(1);
  }

  pass.run(*module);
  dest.flush();

  std::string command = "clang -o " + module->getName().str() + " " + ojectPath.string();
  std::system(command.c_str());
  std::filesystem::remove(ojectPath);
}
