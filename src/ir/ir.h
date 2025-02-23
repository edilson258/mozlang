#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

#include "ast.h"
#include "error.h"
#include "result.h"

enum class opcode
{
  loadc = 0x1,
  call = 0x2,
};

class instr
{
public:
  opcode op;

protected:
  instr(opcode o) : op(o) {};
};

class loadc : public instr
{
public:
  uint32_t index;
  loadc(uint32_t i) : instr(opcode::loadc), index(i) {};
};

class call : public instr
{
public:
  call() : instr(opcode::call) {};
};

enum class object_t
{
  string = 1,
};

using object_v = std::variant<std::monostate, std::string>;

class object
{
public:
  object_t type;
  object_v value;

  object(object_t t, object_v v) : type(t), value(v) {};

  std::string inspect() const
  {
    return std::get<std::string>(value);
  }
};

class const_pool
{
public:
  std::vector<object> pool;
};

class ir
{
public:
  ir() : code() {};

  const_pool pool;
  std::vector<std::shared_ptr<instr>> code;
};

class ir_generator
{
public:
  ir_generator(ast &t) : tree(t), ir_() {};

  result<ir, error> emit();

private:
  ast &tree;
  ir ir_;

  std::optional<error> emit_stmt(std::shared_ptr<stmt>);
  std::optional<error> emit_expr(std::shared_ptr<expr>);
  std::optional<error> emit_expr_call(std::shared_ptr<expr_call>);
  std::optional<error> emit_expr_ident(std::shared_ptr<expr_ident>);
  std::optional<error> emit_expr_string(std::shared_ptr<expr_string>);
};

class ir_disassembler
{
public:
  ir_disassembler(ir i) : ir_(i) {};

  result<std::string, error> disassemble();

private:
  ir ir_;

  std::ostringstream oss;

  std::optional<error> dis_loadc(std::shared_ptr<loadc>);
  std::optional<error> dis_call(std::shared_ptr<call>);
};
