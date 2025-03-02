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

class IRGenerator
{
public:
  IRGenerator(const AST &t) : m_AST(t), m_Program(), m_Globals(), m_CurrentFunction() {};

  Result<lib::Program, ERROR> Emit();

private:
  const AST &m_AST;
  lib::Program m_Program;
  std::map<std::string, uint32_t> m_Globals;
  std::optional<IRGenFunction> m_CurrentFunction;

  uint32_t SaveLocal(std::string);
  uint32_t FindLocal(std::string);

  void EnterFunction(uint32_t, std::string);
  void LeaveFunction();
  void PushInstruction(std::shared_ptr<lib::Instruction>);

  std::optional<ERROR> EmitStatement(std::shared_ptr<Statement>);
  std::optional<ERROR> EmitStatementFunction(std::shared_ptr<StatementFunction>);
  std::optional<ERROR> EmitStatementBlock(std::shared_ptr<StatementBlock>);
  std::optional<ERROR> EmitStatementReturn(std::shared_ptr<StatementReturn>);
  std::optional<ERROR> EmitExpression(std::shared_ptr<Expression>);
  std::optional<ERROR> EmitExpressionCall(std::shared_ptr<ExpressionCall>);
  std::optional<ERROR> EmitExpressionIdentifier(std::shared_ptr<ExpressionIdentifier>);
  std::optional<ERROR> EmitExpressionString(std::shared_ptr<ExpressionString>);
};

class IRDisassembler
{
public:
  IRDisassembler(const lib::Program &program) : m_Program(program), m_Output(), TabRate(4), TabSize(0) {};

  Result<std::string, ERROR> Disassemble();

private:
  const lib::Program &m_Program;
  std::ostringstream m_Output;

  size_t TabRate;
  size_t TabSize;

  void Tab();
  void UnTab();
  void Write(std::string);
  void Writeln(std::string);

  std::optional<ERROR> DisassembleBytecode(lib::ByteCode);
  std::optional<ERROR> DisassembleLoadc(std::shared_ptr<lib::InstructionLoadc>);
  std::optional<ERROR> DisassembleStore(std::shared_ptr<lib::InstructionStore>);
  std::optional<ERROR> DisassembleCall(std::shared_ptr<lib::InstructionCall>);
  std::optional<ERROR> DisassembleReturn(std::shared_ptr<lib::InstructionReturn>);
};
