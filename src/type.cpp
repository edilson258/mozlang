#include "type.h"
#include <cstddef>
#include <memory>
#include <sstream>
#include <string>

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
  case Base::F_STRING:
    return "format string";
  case Base::FUNCTION:
    return "function";
  case Base::ANY:
    return "any";
  case Base::OBJECT:
    return "object";
  }
  return "UNKNWON TYPE";
}

bool Type::IsCompatibleWith(std::shared_ptr<Type> other) const
{
  if (Base::ANY == m_Base)
  {
    return true;
  }
  if ((m_Base == Base::STRING && other.get()->m_Base == Base::F_STRING) || (other.get()->m_Base == Base::STRING && m_Base == Base::F_STRING))
  {
    return true;
  }
  return m_Base == other.get()->m_Base;
}

bool Function::IsCompatibleWith(std::shared_ptr<Type> other) const
{
  if (Base::FUNCTION != other.get()->m_Base)
  {
    return false;
  }
  std::shared_ptr<Function> otherFn = std::static_pointer_cast<Function>(other);
  if ((m_IsVariadicArguments && !otherFn.get()->m_IsVariadicArguments) || (!m_IsVariadicArguments && otherFn.get()->m_IsVariadicArguments))
  {
    return false;
  }
  if ((m_ReqArgsCount != otherFn.get()->m_ReqArgsCount) || (m_Arguments.size() != otherFn.get()->m_Arguments.size()))
  {
    return false;
  }
  if (!m_ReturnType.get()->IsCompatibleWith(otherFn.get()->m_ReturnType))
  {
    return false;
  }
  for (size_t i = 0; i < m_Arguments.size(); ++i)
  {
    if (!m_Arguments.at(i).get()->IsCompatibleWith(otherFn.get()->m_Arguments.at(i)))
    {
      return false;
    }
  }
  return true;
}

bool Object::IsCompatibleWith(std::shared_ptr<Type> other) const
{
  if (Base::OBJECT != other.get()->m_Base)
  {
    return false;
  }
  auto otherObject = std::static_pointer_cast<Object>(other);
  if (m_Entries.size() > otherObject.get()->m_Entries.size())
  {
    return false;
  }
  for (auto &pair : m_Entries)
  {
    if (otherObject.get()->m_Entries.find(pair.first) == otherObject.get()->m_Entries.end())
    {
      return false;
    }
    if (!pair.second.get()->IsCompatibleWith(otherObject.get()->m_Entries[pair.first]))
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
  for (size_t i = 0; i < m_Arguments.size(); ++i)
  {
    oss << m_Arguments.at(i).get()->Inspect();
    if (i + 1 < m_Arguments.size())
    {
      oss << ", ";
    }
  }
  oss << ") -> " << m_ReturnType.get()->Inspect();
  return oss.str();
}

std::string Object::Inspect() const
{
  std::ostringstream oss;
  oss << "{";
  size_t cursor = 1;
  for (auto &pair : m_Entries)
  {
    oss << pair.first << ": " << pair.second.get()->Inspect();
    if (cursor < m_Entries.size())
    {
      oss << ", ";
    }
    cursor++;
  }
  oss << "}";
  return oss.str();
};

std::optional<std::shared_ptr<Type>> NarrowTypes(std::shared_ptr<Type> lhs, std::shared_ptr<Type> rhs)
{
  (void)lhs;
  (void)rhs;

  return std::nullopt;
}

std::shared_ptr<Type> Type::make_void()
{
  return std::make_shared<Type>(Type(Base::VOID));
}
std::shared_ptr<Type> Type::make_string()
{
  return std::make_shared<Type>(Type(Base::STRING));
}
std::shared_ptr<Type> Type::make_fstring()
{
  return std::make_shared<Type>(Type(Base::F_STRING));
}
std::shared_ptr<Type> Type::make_i8()
{
  return std::make_shared<Type>(Type(Base::I8));
}
std::shared_ptr<Type> Type::make_i16()
{
  return std::make_shared<Type>(Type(Base::I16));
}
std::shared_ptr<Type> Type::make_i32()
{
  return std::make_shared<Type>(Type(Base::I32));
}
std::shared_ptr<Type> Type::make_i64()
{
  return std::make_shared<Type>(Type(Base::I64));
}
std::shared_ptr<Type> Type::make_u8()
{
  return std::make_shared<Type>(Type(Base::U8));
}
std::shared_ptr<Type> Type::make_u16()
{
  return std::make_shared<Type>(Type(Base::U16));
}
std::shared_ptr<Type> Type::make_u32()
{
  return std::make_shared<Type>(Type(Base::U32));
}
std::shared_ptr<Type> Type::make_u64()
{
  return std::make_shared<Type>(Type(Base::U64));
}
std::shared_ptr<Type> Type::make_f8()
{
  return std::make_shared<Type>(Type(Base::F8));
}
std::shared_ptr<Type> Type::make_f16()
{
  return std::make_shared<Type>(Type(Base::F16));
}
std::shared_ptr<Type> Type::make_f32()
{
  return std::make_shared<Type>(Type(Base::F32));
}
std::shared_ptr<Type> Type::make_f64()
{
  return std::make_shared<Type>(Type(Base::F64));
}

} // namespace type
