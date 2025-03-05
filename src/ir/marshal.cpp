#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "ir/lib.h"
#include "marshal.h"

std::vector<std::byte> IRMarshal::Marshal()
{
  MarshalContantPool();

  return m_Out;
}

void IRMarshal::MarshalContantPool()
{
  WriteByte(static_cast<std::byte>(m_Program.m_Pool.m_Objects.size()));
  for (auto &object : m_Program.m_Pool.m_Objects)
  {
    MarshalObject(object);
  }
}

void IRMarshal::MarshalObject(lib::Object &object)
{
  switch (object.m_Type)
  {
  case lib::ObjectType::UTF_8:
  case lib::ObjectType::STRING:
    WriteByte(static_cast<std::byte>(object.m_Type));
  }
  std::string &content = std::get<std::string>(object.m_Value);
  size_t contentLen = content.length();
  WriteByte(static_cast<std::byte>(contentLen));
  // WriteUint32(static_cast<uint32_t>(contentLen));
  for (size_t i = 0; i < contentLen; ++i)
  {
    WriteByte(static_cast<std::byte>(content.c_str()[i]));
  }
}

void IRMarshal::WriteByte(std::byte b)
{
  m_Out.push_back(b);
}

void IRMarshal::WriteUint32(uint32_t)
{
}
