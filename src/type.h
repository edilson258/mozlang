#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <vector>

#include "pointer.h"

namespace type
{
enum class Base
{
  VOID,
  STRING,
  FUNCTION,
  I8,
  I16,
  I32,
  I64,
  U8,
  U16,
  U32,
  U64,
  Float,
  OBJECT,
  // internal
  IntRange,
  UNIT,
  UNKNOWN,
};

class Type
{
public:
  Base m_Base;

  Type(Base base) : m_Base(base) {}
  virtual ~Type() = default;

  virtual std::string Inspect() const;
  virtual bool IsCompatWith(Ptr<Type>) const;

  bool IsVoid() const;
  bool IsUnit() const;
  bool Isknown() const;
  bool IsUnknown() const;
  bool IsInteger() const;
  bool IsIntRange() const;
  bool IsSomething() const;
  bool IsNothing() const;
};

class IntRange : public Type
{
public:
  IntRange(bool sign, unsigned long byteSize) : Type(Base::IntRange), m_Signed(sign), m_BytesCout(byteSize) {}

  std::string Inspect() const override;

  Ptr<Type> GetDefault() const;
  Ptr<Type> GetSynthesized() const;
  bool IsSigned() const { return m_Signed; }
  unsigned long GetByteSize() const { return m_BytesCout; }
  bool CanFitIn(const Ptr<Type> other) const;
  bool CanFitIn(const Type *other) const;

private:
  bool m_Signed;
  unsigned long m_BytesCout;
};

class Function : public Type
{
public:
  size_t m_ReqArgsCount;
  std::vector<Ptr<Type>> m_Args;
  Ptr<Type> m_RetType;
  bool m_IsVarArgs;

  std::string Inspect() const override;
  bool IsCompatWith(Ptr<Type>) const override;

  Function(size_t reqArgsCount, std::vector<Ptr<Type>> args, Ptr<Type> retType, bool isVariadic = false) : Type(Base::FUNCTION), m_ReqArgsCount(reqArgsCount), m_Args(std::move(args)), m_RetType(retType), m_IsVarArgs(isVariadic) {}
};

class Object : public Type
{
public:
  std::map<std::string, Ptr<Type>> m_Entries;

  std::string Inspect() const override;
  bool IsCompatWith(Ptr<Type>) const override;

  Object() : Type(Base::OBJECT), m_Entries() {};
};
} // namespace type
