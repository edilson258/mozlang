#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <sys/types.h>

#include "ast.h"
#include "error.h"
#include "ir/lib.h"
#include "result.h"

class IRGenFunction
{
public:
  uint32_t m_Arity;
  std::string m_Name;
  lib::ByteCode m_Code;
  std::map<std::string, uint32_t> m_Locals;

  IRGenFunction(uint32_t arity, std::string name) : m_Arity(arity), m_Name(name), m_Code(), m_Locals() {}
};

class Symbol
{
public:
  uint32_t m_Index;
  bool m_IsGlobal;

  Symbol(uint32_t id, bool isGlobal) : m_Index(id), m_IsGlobal(isGlobal) {}
};

class IRGenerator
{
public:
  IRGenerator(const AST &t) : m_AST(t), m_Program(), m_Globals(), m_Constants(), m_CurrentFunction() {};

  Result<lib::Program, Error> Emit();

private:
  const AST &m_AST;
  lib::Program m_Program;
  std::map<std::string, uint32_t> m_Globals;
  std::map<std::string, uint32_t> m_Constants;
  std::optional<IRGenFunction> m_CurrentFunction;

  uint32_t SaveLocal(std::string);
  std::optional<Symbol> ResolveName(std::string);

  uint32_t SaveIdentifierConstIfNotExist(std::string);

  void EnterFunction(uint32_t, std::string);
  void LeaveFunction();
  void PushInstruction(std::shared_ptr<lib::Instruction>);

  void EmitStatement(std::shared_ptr<Statement>);
  void EmitStatementFunction(std::shared_ptr<StatementFunction>);
  void EmitStatementBlock(std::shared_ptr<StatementBlock>);
  void EmitStatementReturn(std::shared_ptr<StatementReturn>);
  void EmitStatementLet(std::shared_ptr<StatementLet>);
  void EmitExpression(std::shared_ptr<Expression>);
  void EmitExpressionCall(std::shared_ptr<ExpressionCall>);
  void EmitExpressionIdentifier(std::shared_ptr<ExpressionIdentifier>);
  void EmitExpressionString(std::shared_ptr<ExpressionString>);
};

class IRDisassembler
{
public:
  IRDisassembler(const lib::Program &program) : m_Program(program), m_Output(), TabRate(4), TabSize(0) {};

  Result<std::string, Error> Disassemble();

private:
  const lib::Program &m_Program;
  std::ostringstream m_Output;

  size_t TabRate;
  size_t TabSize;

  void Tab();
  void UnTab();
  void Write(std::string);
  void Writeln(std::string);

  std::string InspetObjectType(lib::ObjectType);

  void DisassembleConstantPool();
  void DisassembleBytecode(lib::ByteCode);
  void DisassembleLoadLocal(std::shared_ptr<lib::InstructionLoad>);
  void DisassembleLoadGlobal(std::shared_ptr<lib::InstructionLoadGlobal>);
  void DisassembleLoadConst(std::shared_ptr<lib::InstructionLoadConst>);
  void DisassembleStore(std::shared_ptr<lib::InstructionStoreLocal>);
  void DisassembleCall(std::shared_ptr<lib::InstructionCall>);
  void DisassembleReturn(std::shared_ptr<lib::InstructionReturn>);
};
