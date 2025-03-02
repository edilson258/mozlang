#include <cassert>
#include <cmath>
#include <cstdint>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <sys/types.h>

#include "ast.h"
#include "error.h"
#include "ir.h"
#include "ir/lib.h"
#include "result.h"

Result<lib::Program, ERROR> IRGenerator::Emit()
{
  for (auto statement : m_AST.m_Program)
  {
    auto error = EmitStatement(statement);
    if (error.has_value())
    {
      return Result<lib::Program, ERROR>(error.value());
    }
  }

  return Result<lib::Program, ERROR>(m_Program);
}

std::optional<ERROR> IRGenerator::EmitStatement(std::shared_ptr<Statement> statement)
{
  switch (statement.get()->m_Type)
  {
  case StatementType::BLOCK:
    return EmitStatementBlock(std::static_pointer_cast<StatementBlock>(statement));
  case StatementType::RETURN:
    return EmitStatementReturn(std::static_pointer_cast<StatementReturn>(statement));
  case StatementType::FUNCTION:
    return EmitStatementFunction(std::static_pointer_cast<StatementFunction>(statement));
  case StatementType::EXPRESSION:
    return EmitExpression(std::static_pointer_cast<Expression>(statement));
  }
  return std::nullopt;
}

std::optional<ERROR> IRGenerator::EmitStatementFunction(std::shared_ptr<StatementFunction> functionStatement)
{
  EnterFunction(static_cast<uint32_t>(functionStatement.get()->m_Params.m_Params.size()), functionStatement.get()->m_Identifier.get()->m_Value);

  for (auto &param : functionStatement.get()->m_Params.m_Params)
  {
    PushInstruction(std::make_shared<lib::Instruction>(lib::InstructionStore(SaveLocal(param.m_Identifier->m_Value))));
  }

  auto error = EmitStatementBlock(functionStatement.get()->m_Body);
  LeaveFunction();
  return error;
}

std::optional<ERROR> IRGenerator::EmitStatementBlock(std::shared_ptr<StatementBlock> blockStatement)
{
  std::optional<ERROR> error;
  for (auto &stmt : blockStatement.get()->m_Stmts)
  {
    error = EmitStatement(stmt);
    if (error.has_value())
    {
      return error;
    }
  }
  return error;
}

std::optional<ERROR> IRGenerator::EmitStatementReturn(std::shared_ptr<StatementReturn> returnStatement)
{
  if (returnStatement.get()->m_Value.has_value())
  {
    auto error = EmitExpression(returnStatement.get()->m_Value.value());
    if (error.has_value())
    {
      return error;
    }
  }
  PushInstruction(std::make_shared<lib::InstructionReturn>(lib::InstructionReturn()));
  return std::nullopt;
}

std::optional<ERROR> IRGenerator::EmitExpression(std::shared_ptr<Expression> expression)
{
  switch (expression.get()->m_Type)
  {
  case ExpressionType::CALL:
    return EmitExpressionCall(std::static_pointer_cast<ExpressionCall>(expression));
  case ExpressionType::STRING:
    return EmitExpressionString(std::static_pointer_cast<ExpressionString>(expression));
  case ExpressionType::IDENTIFIER:
    return EmitExpressionIdentifier(std::static_pointer_cast<ExpressionIdentifier>(expression));
  }
  return std::nullopt;
}

std::optional<ERROR> IRGenerator::EmitExpressionCall(std::shared_ptr<ExpressionCall> callExpression)
{
  for (auto argument : callExpression.get()->m_Arguments)
  {
    auto error = EmitExpression(argument);
    if (error.has_value())
    {
      return error.value();
    }
  }
  auto error = EmitExpression(callExpression.get()->m_Callee);
  if (error.has_value())
  {
    return error.value();
  }
  PushInstruction(std::make_shared<lib::InstructionCall>());
  return std::nullopt;
}

std::optional<ERROR> IRGenerator::EmitExpressionIdentifier(std::shared_ptr<ExpressionIdentifier> identifierExpression)
{
  PushInstruction(std::make_shared<lib::InstructionLoadc>(lib::InstructionLoadc(FindLocal(identifierExpression.get()->m_Value))));
  return std::nullopt;
}

std::optional<ERROR> IRGenerator::EmitExpressionString(std::shared_ptr<ExpressionString> exp_string)
{
  uint32_t index = m_Program.m_Pool.Save(lib::Object(lib::ObjectType::STRING, exp_string.get()->m_Value));
  PushInstruction(std::make_shared<lib::InstructionLoadc>(lib::InstructionLoadc(index)));
  return std::nullopt;
}

void IRGenerator::EnterFunction(uint32_t arity, std::string name)
{
  m_CurrentFunction.emplace(IRGenFunction(arity, name));
}

void IRGenerator::LeaveFunction()
{
  uint32_t nameIndex = m_Program.m_Pool.Save(lib::Object(lib::ObjectType::STRING, m_CurrentFunction.value().m_Name));
  m_Program.m_Functions[m_CurrentFunction.value().m_Name] = lib::Function(m_CurrentFunction.value().m_Arity, nameIndex, std::move(m_CurrentFunction.value().m_Code));
  m_CurrentFunction.reset();
};

void IRGenerator::PushInstruction(std::shared_ptr<lib::Instruction> instruction)
{
  if (m_CurrentFunction.has_value())
  {
    m_CurrentFunction.value().m_Code.push_back(instruction);
  }
  else
  {
    m_Program.m_Code.push_back(instruction);
  }
}

uint32_t IRGenerator::SaveLocal(std::string name)
{
  if (m_CurrentFunction.has_value())
  {
    uint32_t index = static_cast<uint32_t>(m_CurrentFunction.value().m_Locals.size());
    m_CurrentFunction.value().m_Locals[name] = index;
    return index;
  }
  uint32_t index = static_cast<uint32_t>(m_Globals.size());
  m_Globals[name] = index;
  return index;
}

uint32_t IRGenerator::FindLocal(std::string name)
{
  if (m_CurrentFunction.has_value() && m_CurrentFunction.value().m_Locals.find(name) != m_CurrentFunction.value().m_Locals.end())
  {
    return m_CurrentFunction.value().m_Locals[name];
  }
  return m_Globals[name];
}

Result<std::string, ERROR> IRDisassembler::Disassemble()
{
  DisassembleBytecode(m_Program.m_Code);
  for (auto &pair : m_Program.m_Functions)
  {
    Writeln(std::format("fun {}:", ""));
    Tab();
    DisassembleBytecode(pair.second.m_Code);
    UnTab();
  }
  return Result<std::string, ERROR>(m_Output.str());
}

std::optional<ERROR> IRDisassembler::DisassembleBytecode(lib::ByteCode byteCode)
{
  std::optional<ERROR> error;
  for (auto &instruction : byteCode)
  {
    switch (instruction.get()->m_OPCode)
    {
    case lib::OPCode::LOAD:
      error = DisassembleLoadc(std::static_pointer_cast<lib::InstructionLoadc>(instruction));
      break;
    case lib::OPCode::STORE:
      error = DisassembleStore(std::static_pointer_cast<lib::InstructionStore>(instruction));
      break;
    case lib::OPCode::CALL:
      error = DisassembleCall(std::static_pointer_cast<lib::InstructionCall>(instruction));
      break;
    case lib::OPCode::RETURN:
      error = DisassembleReturn(std::static_pointer_cast<lib::InstructionReturn>(instruction));
      break;
    }
  }
  return error;
}

std::optional<ERROR> IRDisassembler::DisassembleLoadc(std::shared_ptr<lib::InstructionLoadc> ldc)
{
  Writeln(std::format("loadc '{}'\t", ldc.get()->m_Index));
  return std::nullopt;
}

std::optional<ERROR> IRDisassembler::DisassembleStore(std::shared_ptr<lib::InstructionStore> store)
{
  Writeln(std::format("store '{}'", store.get()->m_Index));
  return std::nullopt;
}

std::optional<ERROR> IRDisassembler::DisassembleCall(std::shared_ptr<lib::InstructionCall>)
{
  Writeln("call");
  return std::nullopt;
}

std::optional<ERROR> IRDisassembler::DisassembleReturn(std::shared_ptr<lib::InstructionReturn>)
{
  Writeln("ret");
  return std::nullopt;
}

void IRDisassembler::Tab()
{
  TabSize += TabRate;
}

void IRDisassembler::UnTab()
{
  TabSize -= TabRate;
}

void IRDisassembler::Write(std::string str)
{
  m_Output << std::string(TabSize, ' ') << str;
}

void IRDisassembler::Writeln(std::string str)
{
  Write(str);
  m_Output << '\n';
}
