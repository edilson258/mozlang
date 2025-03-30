#include <cstddef>
#include <cstdio>
#include <sstream>
#include <string>

#include "pointer.h"
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
  case Base::Float:
    return "float";
  case Base::FUNCTION:
    return "function";
  case Base::OBJECT:
    return "object";
  case Base::UNIT:
    return "()";
  case Base::UNKNOWN:
    return "unknown";
  case Base::IntRange:
    return "integer";
  }
  return "UNKNWON TYPE";
}

bool Type::IsVoid() const
{
  return m_Base == Base::VOID;
}

bool Type::Isknown() const
{
  return m_Base != Base::UNKNOWN;
}

bool Type::IsUnknown() const
{
  return m_Base == Base::UNKNOWN;
}

bool Type::IsSomething() const
{
  switch (m_Base)
  {
  case Base::VOID:
  case Base::UNIT:
    return false;
  default:
    return true;
  }
}

bool Type::IsNothing() const
{
  switch (m_Base)
  {
  case Base::VOID:
  case Base::UNIT:
    return true;
  default:
    return false;
  }
}

bool Type::IsUnit() const
{
  return Base::UNIT == m_Base;
}

bool Type::IsSigned() const
{
  if (!IsInteger())
  {
    return false;
  }
  switch (m_Base)
  {
  case Base::I8:
  case Base::I16:
  case Base::I32:
  case Base::I64:
    return true;
  default:
    return false;
  }
}

bool Type::IsInteger() const
{
  // use switch
  switch (m_Base)
  {
  case Base::I8:
  case Base::I16:
  case Base::I32:
  case Base::I64:
  case Base::U8:
  case Base::U16:
  case Base::U32:
  case Base::U64:
    return true;
  default:
    return false;
  }
}

bool Type::IsIntRange() const
{
  return Base::IntRange == m_Base;
}

Ptr<Type> IntRange::GetDefault() const
{
  if (m_BytesCout <= 4)
  {
    return MakePtr(Type(Base::I32));
  }
  return MakePtr(Type(Base::I64));
}

Ptr<Type> IntRange::GetSynthesized() const
{
  if (m_BytesCout <= 1)
  {
    return MakePtr(Type(Base::I8));
  }
  if (m_BytesCout <= 2)
  {
    return MakePtr(Type(Base::I16));
  }
  if (m_BytesCout <= 4)
  {
    return MakePtr(Type(Base::I32));
  }
  return MakePtr(Type(Base::I64));
}

static inline unsigned long integerToSizeInBytes(Base base)
{
  switch (base)
  {
  case Base::I8:
  case Base::U8:
    return 1;
  case Base::I16:
  case Base::U16:
    return 2;
  case Base::I32:
  case Base::U32:
    return 4;
  case Base::I64:
  case Base::U64:
    return 8;
  default:
    return 0;
  }
}

bool IntRange::CanFitIn(const Ptr<Type> other) const
{
  return CanFitIn(other.get());
}

bool IntRange::CanFitIn(const Type *other) const
{
  if (!other->IsInteger())
  {
    return false;
  }
  auto otherSize = integerToSizeInBytes(other->m_Base);
  if (m_IsSigned && !other->IsSigned())
  {
    return false;
  }
  return m_BytesCout <= otherSize;
}

std::string IntRange::Inspect() const
{
  return GetDefault()->Inspect();
}

bool Type::IsCompatWith(Ptr<Type> other) const
{
  if (m_Base == Base::VOID && other->IsUnit())
  {
    return true;
  }
  if (IsInteger() && other->IsIntRange())
  {
    return CastPtr<IntRange>(other)->CanFitIn(this);
  }
  return m_Base == other->m_Base;
}

bool Function::IsCompatWith(Ptr<Type> other) const
{
  if (Base::FUNCTION != other->m_Base)
  {
    return false;
  }
  auto otherFn = CastPtr<Function>(other);
  if (!(!(m_IsVarArgs ^ otherFn->m_IsVarArgs)))
  {
    return false;
  }
  if ((m_ReqArgsCount != otherFn->m_ReqArgsCount) || (m_Args.size() != otherFn->m_Args.size()))
  {
    return false;
  }
  if (!m_RetType->IsCompatWith(otherFn))
  {
    return false;
  }
  for (size_t i = 0; i < m_Args.size(); ++i)
  {
    if (!m_Args.at(i)->IsCompatWith(otherFn->m_Args.at(i)))
    {
      return false;
    }
  }
  return true;
}

bool Object::IsCompatWith(Ptr<Type> other) const
{
  if (Base::OBJECT != other->m_Base)
  {
    return false;
  }
  auto otherObject = CastPtr<Object>(other);
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
    if (!pair.second->IsCompatWith(otherObject->m_Entries[pair.first]))
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
  if (m_IsVarArgs)
  {
    if (m_Args.size() > 0)
    {
      oss << ", ...";
    }
    else
    {
      oss << "...";
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
