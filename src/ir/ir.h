#pragma once

#include <cstddef>
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

enum class OPCode
{
  LOADC = 0x1,
  CALL = 0x2,
};

class Instruction
{
public:
  OPCode Opcode;

protected:
  Instruction(OPCode op) : Opcode(op) {};
};

class LOADC : public Instruction
{
public:
  uint32_t Index;
  LOADC(uint32_t index) : Instruction(OPCode::LOADC), Index(index) {};
};

class CALL : public Instruction
{
public:
  CALL() : Instruction(OPCode::CALL) {};
};

enum class ObjectType
{
  STRING = 1,
};

using ObjectValue = std::variant<std::monostate, std::string>;

class Object
{
public:
  ObjectType Type;
  ObjectValue Value;

  Object(ObjectType type, ObjectValue value) : Type(type), Value(value) {};

  std::string Inspect() const
  {
    return std::get<std::string>(Value);
  }
};

class ConstantPool
{
public:
  std::vector<Object> Objects;

  size_t size() const
  {
    return Objects.size();
  }
};

class IR
{
public:
  IR() : Code() {};

  ConstantPool Pool;
  std::vector<std::shared_ptr<Instruction>> Code;
};

class IRGenerator
{
public:
  IRGenerator(AST &t) : Ast(t), Ir() {};

  result<IR, ERROR> Emit();

private:
  AST &Ast;
  IR Ir;

  std::optional<ERROR> EmitStatement(std::shared_ptr<Statement>);
  std::optional<ERROR> EmitExpression(std::shared_ptr<Expression>);
  std::optional<ERROR> EmitExpressionCall(std::shared_ptr<ExpressionCall>);
  std::optional<ERROR> EmitExpressionIdentifier(std::shared_ptr<ExpressionIdentifier>);
  std::optional<ERROR> EmitExpressionString(std::shared_ptr<ExpressionString>);
};

class IRDisassembler
{
public:
  IRDisassembler(IR ir) : Ir(ir) {};

  result<std::string, ERROR> Disassemble();

private:
  IR Ir;

  std::ostringstream Output;

  std::optional<ERROR> DisassembleLoadc(std::shared_ptr<LOADC>);
  std::optional<ERROR> DisassembleCall(std::shared_ptr<CALL>);
};
