#pragma once

#include "Ast.h"
#include "AstVisitor.h"
#include "TypeSystem.h"

#include <memory>
#include <string>
#include <unordered_map>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>

class CodeGen : public AstVisitor
{
public:
  CodeGen(AST &tree)
      : ast(tree), Module(std::make_unique<llvm::Module>(tree.SourceFilePath.stem().string(), Context)),
        Builder(Context) {};

  void *visit(BlockStatement *) override;
  void *visit(FunctionStatement *) override;
  void *visit(ReturnStatement *) override;
  void *visit(CallExpression *) override;
  void *visit(IdentifierExpression *) override;
  void *visit(StringExpression *) override;
  void *visit(IntegerExpression *) override;

  llvm::Module *Generate();

private:
  AST &ast;

  llvm::LLVMContext Context;
  std::unique_ptr<llvm::Module> Module;
  llvm::IRBuilder<> Builder;

  std::unordered_map<std::string, llvm::Value *> Store;

  void SetupBuiltIns();

  // Helpers
  llvm::Type *MozToLLVMType(Type);
};
