#include <cassert>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include "ir.h"

result<ir, error> ir_generator::emit()
{
  for (auto stmt : tree.program)
  {
    auto err = emit_stmt(stmt);
    if (err.has_value())
    {
      return result<ir, error>(err.value());
    }
  }

  return result<ir, error>(ir_);
}

std::optional<error> ir_generator::emit_stmt(std::shared_ptr<stmt> stm)
{
  switch (stm.get()->type)
  {
  case stmt_t::expr:
    return emit_expr(std::static_pointer_cast<expr>(stm));
  }
  return std::nullopt;
}

std::optional<error> ir_generator::emit_expr(std::shared_ptr<expr> exp)
{
  switch (exp.get()->type)
  {
  case expr_t::call:
    return emit_expr_call(std::static_pointer_cast<expr_call>(exp));
  case expr_t::string:
    return emit_expr_string(std::static_pointer_cast<expr_string>(exp));
  case expr_t::ident:
    return emit_expr_ident(std::static_pointer_cast<expr_ident>(exp));
  }
  return std::nullopt;
}

std::optional<error> ir_generator::emit_expr_call(std::shared_ptr<expr_call> exp_call)
{
  for (auto arg : exp_call.get()->args)
  {
    auto err = emit_expr(arg);
    if (err.has_value())
    {
      return err.value();
    }
  }
  auto err = emit_expr(exp_call.get()->callee);
  if (err.has_value())
  {
    return err.value();
  }
  ir_.code.push_back(std::make_shared<call>());
  return std::nullopt;
}

std::optional<error> ir_generator::emit_expr_ident(std::shared_ptr<expr_ident> exp_ident)
{
  uint32_t index = static_cast<uint32_t>(ir_.pool.pool.size());
  ir_.pool.pool.push_back(object(object_t::string, exp_ident.get()->value));
  ir_.code.push_back(std::make_shared<loadc>(index));
  return std::nullopt;
}

std::optional<error> ir_generator::emit_expr_string(std::shared_ptr<expr_string> exp_string)
{
  uint32_t index = static_cast<uint32_t>(ir_.pool.pool.size());
  ir_.pool.pool.push_back(object(object_t::string, exp_string.get()->value));
  ir_.code.push_back(std::make_shared<loadc>(index));
  return std::nullopt;
}

result<std::string, error> ir_disassembler::disassemble()
{
  for (auto &inst : ir_.code)
  {
    switch (inst.get()->op)
    {
    case opcode::loadc:
    {
      auto err = dis_loadc(std::static_pointer_cast<loadc>(inst));
      if (err.has_value())
      {
        return result<std::string, error>(err.value());
      }
      break;
    }
    case opcode::call:
    {
      auto err = dis_call(std::static_pointer_cast<call>(inst));
      if (err.has_value())
      {
        return result<std::string, error>(err.value());
      }
      break;
    }
    }
  }
  return result<std::string, error>(oss.str());
}

std::optional<error> ir_disassembler::dis_loadc(std::shared_ptr<loadc> ldc)
{
  oss << "load_const " << ldc.get()->index << "          ;;" << ir_.pool.pool.at(ldc.get()->index).inspect() << std::endl;
  return std::nullopt;
}

std::optional<error> ir_disassembler::dis_call(std::shared_ptr<call>)
{
  oss << "call" << std::endl;
  return std::nullopt;
}
