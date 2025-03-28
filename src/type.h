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
  F8,
  F16,
  F32,
  F64,
  OBJECT,
  // internal
  UNIT,
  ERROR,
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
  bool IsError() const;
  bool IsNothing() const;
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
