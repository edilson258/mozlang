#include <cassert>
#include <cmath>
#include <cstddef>
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

Result<lib::Program, Error> IRGenerator::Emit()
{
  for (auto statement : m_AST.m_Program)
  {
    EmitStatement(statement);
  }
  return Result<lib::Program, Error>(m_Program);
}

void IRGenerator::EmitStatement(std::shared_ptr<Statement> statement)
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
}

void IRGenerator::EmitStatementFunction(std::shared_ptr<StatementFunction> functionStatement)
{
  SaveIdentifierConstIfNotExist(functionStatement.get()->m_Identifier.get()->m_Value);
  EnterFunction(static_cast<uint32_t>(functionStatement.get()->m_Params.m_Params.size()), functionStatement.get()->m_Identifier.get()->m_Value);
  for (auto &param : functionStatement.get()->m_Params.m_Params)
  {
    SaveLocal(param.m_Identifier->m_Value);
  }
  EmitStatementBlock(functionStatement.get()->m_Body);
  LeaveFunction();
}

void IRGenerator::EmitStatementBlock(std::shared_ptr<StatementBlock> blockStatement)
{
  for (auto &stmt : blockStatement.get()->m_Stmts)
  {
    EmitStatement(stmt);
  }
}

void IRGenerator::EmitStatementReturn(std::shared_ptr<StatementReturn> returnStatement)
{
  if (returnStatement.get()->m_Value.has_value())
  {
    EmitExpression(returnStatement.get()->m_Value.value());
  }
  PushInstruction(std::make_shared<lib::InstructionReturn>(lib::InstructionReturn()));
}

void IRGenerator::EmitExpression(std::shared_ptr<Expression> expression)
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
}

void IRGenerator::EmitExpressionCall(std::shared_ptr<ExpressionCall> callExpression)
{
  for (auto &argument : callExpression.get()->m_Arguments)
  {
    EmitExpression(argument);
  }
  EmitExpression(callExpression.get()->m_Callee);
  PushInstruction(std::make_shared<lib::InstructionCall>());
}

uint32_t IRGenerator::SaveIdentifierConstIfNotExist(std::string utf8)
{
  if (m_Constants.find(utf8) != m_Constants.end())
  {
    return m_Constants[utf8];
  }
  auto index = m_Program.m_Pool.Save(lib::Object(lib::ObjectType::UTF_8, utf8));
  m_Constants[utf8] = index;
  return index;
}

void IRGenerator::EmitExpressionIdentifier(std::shared_ptr<ExpressionIdentifier> identifierExpression)
{
  std::optional<Symbol> symbOpt = ResolveName(identifierExpression.get()->m_Value);
  if (symbOpt.has_value())
  {
    if (symbOpt.value().m_IsGlobal && m_CurrentFunction.has_value())
    {
      PushInstruction(std::make_shared<lib::InstructionLoadGlobal>(lib::InstructionLoadGlobal(symbOpt.value().m_Index)));
    }
    else
    {
      PushInstruction(std::make_shared<lib::InstructionLoad>(lib::InstructionLoad(symbOpt.value().m_Index)));
    }
  }
  else
  {
    PushInstruction(std::make_shared<lib::InstructionLoadConst>(lib::InstructionLoadConst(SaveIdentifierConstIfNotExist(identifierExpression.get()->m_Value))));
  }
}

void IRGenerator::EmitExpressionString(std::shared_ptr<ExpressionString> stringExpression)
{
  PushInstruction(std::make_shared<lib::InstructionLoadConst>(lib::InstructionLoadConst(m_Program.m_Pool.Save(lib::Object(lib::ObjectType::STRING, stringExpression.get()->m_Value)))));
}

void IRGenerator::EnterFunction(uint32_t arity, std::string name)
{
  m_CurrentFunction.emplace(IRGenFunction(arity, name));
}

void IRGenerator::LeaveFunction()
{
  m_Program.m_Functions[m_CurrentFunction.value().m_Name] = lib::Function(m_CurrentFunction.value().m_Arity, m_Constants[m_CurrentFunction.value().m_Name], std::move(m_CurrentFunction.value().m_Code));
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

std::optional<Symbol> IRGenerator::ResolveName(std::string name)
{
  if (m_CurrentFunction.has_value() && m_CurrentFunction.value().m_Locals.find(name) != m_CurrentFunction.value().m_Locals.end())
  {
    return Symbol(m_CurrentFunction.value().m_Locals[name], false);
  }
  if (m_Globals.find(name) != m_Globals.end())
  {
    return Symbol(m_Globals[name], true);
  }
  return std::nullopt;
}

Result<std::string, Error> IRDisassembler::Disassemble()
{
  DisassembleConstantPool();
  Writeln("Global Instructions:");
  Tab();
  DisassembleBytecode(m_Program.m_Code);
  UnTab();
  Writeln("Function Instructions:");
  Tab();
  for (auto &pair : m_Program.m_Functions)
  {
    Writeln(std::format("fun {}:", m_Program.m_Pool.Get(pair.second.m_NameIndex).value().Inspect()));
    Tab();
    DisassembleBytecode(pair.second.m_Code);
    UnTab();
    Writeln("");
  }
  UnTab();
  return Result<std::string, Error>(m_Output.str());
}

void IRDisassembler::DisassembleConstantPool()
{
  Writeln("Constant Pool:");
  Tab();
  for (size_t i = 0; i < m_Program.m_Pool.m_Objects.size(); ++i)
  {
    Writeln(std::format("{}\t{}\t{}", InspetObjectType(m_Program.m_Pool.m_Objects.at(i).m_Type), m_Program.m_Pool.m_Objects.at(i).Inspect(), i));
  }
  UnTab();
  Writeln("");
}

void IRDisassembler::DisassembleBytecode(lib::ByteCode byteCode)
{
  for (auto &instruction : byteCode)
  {
    switch (instruction.get()->m_OPCode)
    {
    case lib::OPCode::LOAD:
      DisassembleLoadLocal(std::static_pointer_cast<lib::InstructionLoad>(instruction));
      break;
    case lib::OPCode::LOADG:
      DisassembleLoadGlobal(std::static_pointer_cast<lib::InstructionLoadGlobal>(instruction));
      break;
    case lib::OPCode::LOADC:
      DisassembleLoadConst(std::static_pointer_cast<lib::InstructionLoadConst>(instruction));
      break;
    case lib::OPCode::STOREL:
      DisassembleStore(std::static_pointer_cast<lib::InstructionStoreLocal>(instruction));
      break;
    case lib::OPCode::CALL:
      DisassembleCall(std::static_pointer_cast<lib::InstructionCall>(instruction));
      break;
    case lib::OPCode::RETURN:
      DisassembleReturn(std::static_pointer_cast<lib::InstructionReturn>(instruction));
      break;
    }
  }
}

void IRDisassembler::DisassembleLoadLocal(std::shared_ptr<lib::InstructionLoad> ldl)
{
  Writeln(std::format("load\t{}", ldl.get()->m_Index));
}

void IRDisassembler::DisassembleLoadGlobal(std::shared_ptr<lib::InstructionLoadGlobal> ldg)
{
  Writeln(std::format("loadg\t{}", ldg.get()->m_Index));
}

void IRDisassembler::DisassembleLoadConst(std::shared_ptr<lib::InstructionLoadConst> ldc)
{
  Writeln(std::format("loadc\t{}", ldc.get()->m_Index));
}

void IRDisassembler::DisassembleStore(std::shared_ptr<lib::InstructionStoreLocal> store)
{
  Writeln(std::format("store\t{}", store.get()->m_Index));
}

void IRDisassembler::DisassembleCall(std::shared_ptr<lib::InstructionCall>)
{
  Writeln("call");
}

void IRDisassembler::DisassembleReturn(std::shared_ptr<lib::InstructionReturn>)
{
  Writeln("ret");
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

std::string IRDisassembler::InspetObjectType(lib::ObjectType type)
{
  switch (type)
  {
  case lib::ObjectType::STRING:
    return "string";
  case lib::ObjectType::UTF_8:
    return "utf8";
  }
  return "<unknown object type>";
}
