#include <cstddef>
#include <sstream>
#include <string>

#include "type.h"

namespace type
{

std::string Type::Inspect() const
{
  switch (m_Base)
  {
  case Base::VOID:
    return "void";
  case Base::STRING:
    return "string";
  case Base::I8:
    return "i8";
  case Base::I16:
    return "i16";
  case Base::I32:
    return "i32";
  case Base::I64:
    return "i64";
  case Base::U8:
    return "u8";
  case Base::U16:
    return "u16";
  case Base::U32:
    return "u32";
  case Base::U64:
    return "u64";
  case Base::F8:
    return "f8";
  case Base::F16:
    return "f16";
  case Base::F32:
    return "f32";
  case Base::F64:
    return "f64";
  case Base::FUNCTION:
    return "function";
  case Base::ANY:
    return "any";
  case Base::OBJECT:
    return "object";
  }
  return "UNKNWON TYPE";
}

bool Type::IsCompatibleWith(Type *other) const
{
  if (Base::ANY == m_Base)
  {
    return true;
  }
  return m_Base == other->m_Base;
}

bool Function::IsCompatibleWith(Type *other) const
{
  if (Base::FUNCTION != other->m_Base)
  {
    return false;
  }
  auto *otherFn = (Function *)other;
  if (!(!(m_IsVarArgs ^ otherFn->m_IsVarArgs)))
  {
    return false;
  }
  if ((m_ReqArgsCount != otherFn->m_ReqArgsCount) || (m_Args.size() != otherFn->m_Args.size()))
  {
    return false;
  }
  if (!m_RetType->IsCompatibleWith(otherFn))
  {
    return false;
  }
  for (size_t i = 0; i < m_Args.size(); ++i)
  {
    if (!m_Args.at(i)->IsCompatibleWith(otherFn->m_Args.at(i)))
    {
      return false;
    }
  }
  return true;
}

bool Object::IsCompatibleWith(Type *other) const
{
  if (Base::OBJECT != other->m_Base)
  {
    return false;
  }
  auto *otherObject = (Object *)other;
  if (m_Entries.size() > otherObject->m_Entries.size())
  {
    return false;
  }
  for (auto &pair : m_Entries)
  {
    if (otherObject->m_Entries.find(pair.first) == otherObject->m_Entries.end())
    {
      return false;
    }
    if (!pair.second->IsCompatibleWith(otherObject->m_Entries[pair.first]))
    {
      return false;
    }
  }
  return true;
}

std::string Function::Inspect() const
{
  std::ostringstream oss;
  oss << "fun(";
  for (size_t i = 0; i < m_Args.size(); ++i)
  {
    oss << m_Args.at(i)->Inspect();
    if (i + 1 < m_Args.size())
    {
      oss << ", ";
    }
  }
  oss << ") -> " << m_RetType->Inspect();
  return oss.str();
}

std::string Object::Inspect() const
{
  std::ostringstream oss;
  oss << "{";
  size_t cursor = 1;
  for (auto &pair : m_Entries)
  {
    oss << pair.first << ": " << pair.second->Inspect();
    if (cursor < m_Entries.size())
    {
      oss << ", ";
    }
    cursor++;
  }
  oss << "}";
  return oss.str();
};
} // namespace type
