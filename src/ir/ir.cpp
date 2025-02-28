#include <cassert>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include "ir.h"

Result<IR, ERROR> IRGenerator::Emit()
{
  for (auto statement : Ast.m_Program)
  {
    auto error = EmitStatement(statement);
    if (error.has_value())
    {
      return Result<IR, ERROR>(error.value());
    }
  }

  return Result<IR, ERROR>(Ir);
}

std::optional<ERROR> IRGenerator::EmitStatement(std::shared_ptr<Statement> statement)
{
  switch (statement.get()->m_Type)
  {
  case StatementType::BLOCK:
  case StatementType::RETURN:
  case StatementType::FUNCTION:
    break;
  case StatementType::EXPRESSION:
    return EmitExpression(std::static_pointer_cast<Expression>(statement));
  }
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
  Ir.m_Code.push_back(std::make_shared<CALL>());
  return std::nullopt;
}

std::optional<ERROR> IRGenerator::EmitExpressionIdentifier(std::shared_ptr<ExpressionIdentifier> identifierExpression)
{
  uint32_t index = static_cast<uint32_t>(Ir.m_Pool.size());
  Ir.m_Pool.m_Objects.push_back(Object(ObjectType::STRING, identifierExpression.get()->m_Value));
  Ir.m_Code.push_back(std::make_shared<LOADC>(index));
  return std::nullopt;
}

std::optional<ERROR> IRGenerator::EmitExpressionString(std::shared_ptr<ExpressionString> exp_string)
{
  uint32_t index = static_cast<uint32_t>(Ir.m_Pool.size());
  Ir.m_Pool.m_Objects.push_back(Object(ObjectType::STRING, exp_string.get()->m_Value));
  Ir.m_Code.push_back(std::make_shared<LOADC>(index));
  return std::nullopt;
}

Result<std::string, ERROR> IRDisassembler::Disassemble()
{
  for (auto &instruction : Ir.m_Code)
  {
    switch (instruction.get()->m_Opcode)
    {
    case OPCode::LOADC:
    {
      auto err = DisassembleLoadc(std::static_pointer_cast<LOADC>(instruction));
      if (err.has_value())
      {
        return Result<std::string, ERROR>(err.value());
      }
      break;
    }
    case OPCode::CALL:
    {
      auto err = DisassembleCall(std::static_pointer_cast<CALL>(instruction));
      if (err.has_value())
      {
        return Result<std::string, ERROR>(err.value());
      }
      break;
    }
    }
  }
  return Result<std::string, ERROR>(Output.str());
}

std::optional<ERROR> IRDisassembler::DisassembleLoadc(std::shared_ptr<LOADC> ldc)
{
  Output << "load_const " << ldc.get()->m_Index << "          ;;" << Ir.m_Pool.m_Objects.at(ldc.get()->m_Index).Inspect() << std::endl;
  return std::nullopt;
}

std::optional<ERROR> IRDisassembler::DisassembleCall(std::shared_ptr<CALL>)
{
  Output << "call" << std::endl;
  return std::nullopt;
}
