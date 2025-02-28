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
  OPCode m_Opcode;

protected:
  Instruction(OPCode opcode) : m_Opcode(opcode) {};
};

class LOADC : public Instruction
{
public:
  uint32_t m_Index;
  LOADC(uint32_t index) : Instruction(OPCode::LOADC), m_Index(index) {};
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
  ObjectType m_Type;
  ObjectValue m_Value;

  Object(ObjectType type, ObjectValue value) : m_Type(type), m_Value(value) {};

  std::string Inspect() const
  {
    return std::get<std::string>(m_Value);
  }
};

class ConstantPool
{
public:
  std::vector<Object> m_Objects;

  size_t size() const
  {
    return m_Objects.size();
  }
};

class IR
{
public:
  IR() : m_Code() {};

  ConstantPool m_Pool;
  std::vector<std::shared_ptr<Instruction>> m_Code;
};

class IRGenerator
{
public:
  IRGenerator(AST &t) : Ast(t), Ir() {};

  Result<IR, ERROR> Emit();

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

  Result<std::string, ERROR> Disassemble();

private:
  IR Ir;

  std::ostringstream Output;

  std::optional<ERROR> DisassembleLoadc(std::shared_ptr<LOADC>);
  std::optional<ERROR> DisassembleCall(std::shared_ptr<CALL>);
};
