#include <cassert>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include "ir.h"

result<IR, ERROR> IRGenerator::Emit()
{
  for (auto statement : Ast.Program)
  {
    auto error = EmitStatement(statement);
    if (error.has_value())
    {
      return result<IR, ERROR>(error.value());
    }
  }

  return result<IR, ERROR>(Ir);
}

std::optional<ERROR> IRGenerator::EmitStatement(std::shared_ptr<Statement> statement)
{
  switch (statement.get()->Type)
  {
  case StatementType::EXPRESSION:
    return EmitExpression(std::static_pointer_cast<Expression>(statement));
  }
  return std::nullopt;
}

std::optional<ERROR> IRGenerator::EmitExpression(std::shared_ptr<Expression> expression)
{
  switch (expression.get()->Type)
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
  for (auto argument : callExpression.get()->Arguments)
  {
    auto error = EmitExpression(argument);
    if (error.has_value())
    {
      return error.value();
    }
  }
  auto error = EmitExpression(callExpression.get()->Callee);
  if (error.has_value())
  {
    return error.value();
  }
  Ir.Code.push_back(std::make_shared<CALL>());
  return std::nullopt;
}

std::optional<ERROR> IRGenerator::EmitExpressionIdentifier(std::shared_ptr<ExpressionIdentifier> identifierExpression)
{
  uint32_t index = static_cast<uint32_t>(Ir.Pool.size());
  Ir.Pool.Objects.push_back(Object(ObjectType::STRING, identifierExpression.get()->Value));
  Ir.Code.push_back(std::make_shared<LOADC>(index));
  return std::nullopt;
}

std::optional<ERROR> IRGenerator::EmitExpressionString(std::shared_ptr<ExpressionString> exp_string)
{
  uint32_t index = static_cast<uint32_t>(Ir.Pool.Objects.size());
  Ir.Pool.Objects.push_back(Object(ObjectType::STRING, exp_string.get()->Value));
  Ir.Code.push_back(std::make_shared<LOADC>(index));
  return std::nullopt;
}

result<std::string, ERROR> IRDisassembler::Disassemble()
{
  for (auto &instruction : Ir.Code)
  {
    switch (instruction.get()->Opcode)
    {
    case OPCode::LOADC:
    {
      auto err = DisassembleLoadc(std::static_pointer_cast<LOADC>(instruction));
      if (err.has_value())
      {
        return result<std::string, ERROR>(err.value());
      }
      break;
    }
    case OPCode::CALL:
    {
      auto err = DisassembleCall(std::static_pointer_cast<CALL>(instruction));
      if (err.has_value())
      {
        return result<std::string, ERROR>(err.value());
      }
      break;
    }
    }
  }
  return result<std::string, ERROR>(Output.str());
}

std::optional<ERROR> IRDisassembler::DisassembleLoadc(std::shared_ptr<LOADC> ldc)
{
  Output << "load_const " << ldc.get()->Index << "          ;;" << Ir.Pool.Objects.at(ldc.get()->Index).Inspect() << std::endl;
  return std::nullopt;
}

std::optional<ERROR> IRDisassembler::DisassembleCall(std::shared_ptr<CALL>)
{
  Output << "call" << std::endl;
  return std::nullopt;
}
