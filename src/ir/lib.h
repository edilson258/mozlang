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
enum class OPCode
{
  LOAD = 0x01,
  STORE = 0x02,
  CALL = 0x03,
  RETURN = 0x04,
};

class Instruction
{
public:
  OPCode m_OPCode;

protected:
  Instruction(OPCode opcode) : m_OPCode(opcode) {};
};

class InstructionLoadc : public Instruction
{
public:
  uint32_t m_Index;
  InstructionLoadc(uint32_t index) : Instruction(OPCode::LOAD), m_Index(index) {};
};

class InstructionStore : public Instruction
{
public:
  uint32_t m_Index;
  InstructionStore(uint32_t index) : Instruction(OPCode::STORE), m_Index(index) {};
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

class Pool
{
public:
  uint32_t Save(Object);
  std::optional<Object> Get(uint32_t) const;

  Pool() : m_Objects() {};

private:
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
