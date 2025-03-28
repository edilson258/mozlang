#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <vector>

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
  // internal
  ANY,
  OBJECT,
};

class Type
{
public:
  Base m_Base;

  Type(Base base) : m_Base(base) {}
  virtual ~Type() = default;

  virtual std::string Inspect() const;
  virtual bool IsCompatibleWith(Type *) const;
};

class Function : public Type
{
public:
  size_t m_ReqArgsCount;
  std::vector<Type *> m_Args;
  Type *m_RetType;
  bool m_IsVarArgs;

  std::string Inspect() const override;
  bool IsCompatibleWith(Type *) const override;

  Function(size_t reqArgsCount, std::vector<Type *> args, Type *retType, bool isVariadic = false) : Type(Base::FUNCTION), m_ReqArgsCount(reqArgsCount), m_Args(std::move(args)), m_RetType(retType), m_IsVarArgs(isVariadic) {}
};

class Object : public Type
{
public:
  std::map<std::string, Type *> m_Entries;

  std::string Inspect() const override;
  bool IsCompatibleWith(Type *) const override;

  Object() : Type(Base::OBJECT), m_Entries() {};
};
} // namespace type
