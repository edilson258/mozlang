#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "ir/lib.h"

class IRMarshal
{
public:
  IRMarshal(lib::Program &program) : m_Program(program) {};

  std::vector<std::byte> Marshal();

private:
  lib::Program &m_Program;
  std::vector<std::byte> m_Out;

  void WriteByte(std::byte);
  void WriteUint32(uint32_t);

  void MarshalContantPool();
  void MarshalObject(lib::Object &);
};
