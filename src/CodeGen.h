#include "Ast.h"
#include "AstVisitor.h"

#include <llvm-18/llvm/IR/Value.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

#include <memory>
#include <string>
#include <unordered_map>

class CodeGen : public AstVisitor
{
public:
  CodeGen() : Module(std::make_unique<llvm::Module>("mzlang", Context)), Builder(Context) {};

  void *visit(BlockStatement *) override;
  void *visit(FunctionStatement *) override;
  void *visit(CallExpression *) override;
  void *visit(IdentifierExpression *) override;
  void *visit(StringExpression *) override;

  llvm::Module *Generate(AST &ast);

private:
  llvm::LLVMContext Context;
  std::unique_ptr<llvm::Module> Module;
  llvm::IRBuilder<> Builder;

  std::unordered_map<std::string, llvm::Value *> Store;

  void SetupBuiltIns();
};
