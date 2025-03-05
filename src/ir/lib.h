#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <stdint.h>
#include <string>
#include <variant>
#include <vector>

namespace lib
{

enum class ObjectType
{
  UTF_8 = 0x01,
  STRING = 0x02,
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

enum class OPCode
{
  /* Load symbol from locals */
  LOAD = 0x01,
  /* Load symbol from globals */
  LOADG = 0x02,
  /* Load constant value from constant pool */
  LOADC = 0x03,
  STOREL = 0x04,
  CALL = 0x05,
  RETURN = 0x06,
};

class Instruction
{
public:
  OPCode m_OPCode;

protected:
  Instruction(OPCode opcode) : m_OPCode(opcode) {};
};

class InstructionLoad : public Instruction
{
public:
  uint32_t m_Index;
  InstructionLoad(uint32_t index) : Instruction(OPCode::LOAD), m_Index(index) {};
};

class InstructionLoadGlobal : public Instruction
{
public:
  uint32_t m_Index;
  InstructionLoadGlobal(uint32_t index) : Instruction(OPCode::LOADG), m_Index(index) {};
};

class InstructionLoadConst : public Instruction
{
public:
  uint32_t m_Index;
  InstructionLoadConst(uint32_t index) : Instruction(OPCode::LOADC), m_Index(index) {};
};

class InstructionStoreLocal : public Instruction
{
public:
  uint32_t m_Index;
  InstructionStoreLocal(uint32_t index) : Instruction(OPCode::STOREL), m_Index(index) {};
};

class InstructionCall : public Instruction
{
public:
  InstructionCall() : Instruction(OPCode::CALL) {};
};

class InstructionReturn : public Instruction
{
public:
  InstructionReturn() : Instruction(OPCode::RETURN) {};
};

class Pool
{
public:
  uint32_t Save(Object);
  std::optional<Object> Get(uint32_t) const;

  Pool() : m_Objects() {};

  std::vector<Object> m_Objects;
};

using ByteCode = std::vector<std::shared_ptr<Instruction>>;

class Function
{
public:
  uint32_t m_Arity;
  uint32_t m_NameIndex;
  ByteCode m_Code;

  Function() : m_Arity(0), m_NameIndex(0), m_Code() {};
  Function(uint32_t arity, uint32_t nameIndex, ByteCode code) : m_Arity(arity), m_NameIndex(nameIndex), m_Code(std::move(code)) {};
};

class Program
{
public:
  Pool m_Pool;
  ByteCode m_Code;
  std::map<std::string, Function> m_Functions;

  Program() : m_Pool(), m_Code(), m_Functions() {};
  Program(Pool pool, ByteCode code, std::map<std::string, Function> functions) : m_Pool(pool), m_Code(code), m_Functions(std::move(functions)) {};
};

}; // namespace lib
