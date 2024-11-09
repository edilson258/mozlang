#include "CodeGen.h"
#include "Ast.h"
#include "TypeSystem.h"

#include <cstdint>
#include <cstdlib>
#include <memory>
#include <regex>
#include <string>
#include <vector>

#include <llvm-18/llvm/ADT/Any.h>
#include <llvm-18/llvm/IR/BasicBlock.h>
#include <llvm-18/llvm/IR/DerivedTypes.h>
#include <llvm-18/llvm/IR/Function.h>
#include <llvm-18/llvm/IR/LLVMContext.h>
#include <llvm-18/llvm/IR/Module.h>
#include <llvm-18/llvm/IR/Type.h>
#include <llvm-18/llvm/IR/Value.h>
#include <llvm-18/llvm/Support/Casting.h>
#include <llvm-18/llvm/Support/raw_ostream.h>

llvm::Function *makePrintf(llvm::LLVMContext &c, llvm::Module *m);
void CodeGen::SetupBuiltIns() { Store["print"] = makePrintf(Context, Module.get()); }

llvm::Module *CodeGen::Generate()
{
  SetupBuiltIns();
  for (auto &node : ast.Nodes)
  {
    node->accept(this);
  }
  return Module.get();
}

llvm::Type *CodeGen::MozToLLVMType(Type type)
{
  switch (type.Base)
  {
  case BaseType::Integer:
    return Builder.getInt32Ty();
  case BaseType::String:
    return Builder.getInt8Ty()->getPointerTo();
  case BaseType::Void:
    return Builder.getVoidTy();
  default:
    llvm::errs() << "[ERROR]: Cannot convert provided type to llvm\n";
    std::exit(1);
  }
}

void *CodeGen::visit(FunctionStatement *fnStmt)
{
  std::vector<llvm::Type *> paramTypes;
  for (auto &param : fnStmt->Params)
  {
    paramTypes.push_back(MozToLLVMType(*param.TypeAnnotation.Type));
  }
  llvm::FunctionType *fnType = llvm::FunctionType::get(MozToLLVMType(*fnStmt->ReturnType.Type), paramTypes, false);
  llvm::Function *fn =
      llvm::Function::Create(fnType, llvm::Function::ExternalLinkage, fnStmt->Identifier->GetValue(), Module.get());
  llvm::BasicBlock *fnBody = llvm::BasicBlock::Create(Context, "entry", fn);

  Store[fnStmt->Identifier->GetValue()] = fn;

  Builder.SetInsertPoint(fnBody);

  for (unsigned int i = 0; i < fn->arg_size(); ++i)
  {
    Store[fnStmt->Params.at(i).Identifier->GetValue()] = fn->getArg(i);
  }

  fnStmt->Body.accept(this);

  return nullptr;
}

void *CodeGen::visit(ReturnStatement *retStmt)
{
  if (retStmt->Value.has_value())
  {
    llvm::Value *value = static_cast<llvm::Value *>(retStmt->Value.value()->accept(this));
    Builder.CreateRet(value);
  }
  else
  {
    Builder.CreateRetVoid();
  }
  return nullptr;
}

void *CodeGen::visit(BlockStatement *blockStmt)
{
  for (auto &stmt : blockStmt->Statements)
  {
    stmt->accept(this);
  }
  return nullptr;
}

void *CodeGen::visit(CallExpression *callExpr)
{
  std::vector<llvm::Value *> args;
  for (auto &arg : callExpr->Args.Args)
  {
    // TODO: validate the value before push
    args.push_back(static_cast<llvm::Value *>(arg->accept(this)));
  }
  llvm::Value *callee = static_cast<llvm::Value *>(callExpr->Callee->accept(this));
  if (!callee)
  {
    llvm::errs() << "[ERROR]: Invalid callee\n";
    std::exit(1);
  }
  if (llvm::dyn_cast<llvm::Function>(callee))
  {
    return Builder.CreateCall(static_cast<llvm::Function *>(callee), args);
  }
  else
  {
    llvm::errs() << "[ERROR]: Callee is not callable\n";
    std::exit(1);
  }
}

void *CodeGen::visit(IdentifierExpression *ident)
{
  if (Store.find(ident->GetValue()) != Store.end())
  {
    return Store[ident->GetValue()];
  }
  llvm::errs() << "[ERROR]: Unbound name: " << ident->GetValue() << "\n";
  std::exit(1);
}

void *CodeGen::visit(StringExpression *strExpr)
{
  std::string newLineScapedStr = std::regex_replace(strExpr->GetValue(), std::regex(R"(\\n)"), "\n");
  return Builder.CreateGlobalStringPtr(newLineScapedStr);
}

void *CodeGen::visit(IntegerExpression *intExpr)
{
  return Builder.getInt32(static_cast<uint32_t>(intExpr->GetValue()));
}

llvm::Function *makePrintf(llvm::LLVMContext &c, llvm::Module *m)
{
  std::vector<llvm::Type *> args = {llvm::Type::getInt8Ty(c)->getPointerTo()};
  llvm::FunctionType *type       = llvm::FunctionType::get(llvm::Type::getInt32Ty(c), args, true);
  llvm::Function *fn             = llvm::Function::Create(type, llvm::Function::ExternalLinkage, "printf", m);
  return fn;
}
