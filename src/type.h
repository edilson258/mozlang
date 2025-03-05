#pragma once

#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace type
{
enum class Base
{
  VOID,
  STRING,
  FUNCTION,

  // integers
  I32,

  // internal
  ANY,
  F_STRING,
  OBJECT,
};

class Type
{
public:
  Base m_Base;

  Type(Base base) : m_Base(base) {}
};

class Function : public Type
{
public:
  size_t m_ReqArgsCount;
  std::vector<std::shared_ptr<Type>> m_Arguments;
  std::shared_ptr<Type> m_ReturnType;
  bool m_IsVariadicArguments;

  Function(size_t reqArgsCount, std::vector<std::shared_ptr<Type>> arguments, std::shared_ptr<Type> returnType, bool isVariadic = false) : Type(Base::FUNCTION), m_ReqArgsCount(reqArgsCount), m_Arguments(std::move(arguments)), m_ReturnType(returnType), m_IsVariadicArguments(isVariadic) {}
};

class Object : public Type
{
public:
  std::map<std::string, std::shared_ptr<Type>> m_Entries;

  Object() : Type(Base::OBJECT) {};
};

std::string InspectBase(Base);
bool MatchBaseTypes(Base, Base);
std::optional<std::shared_ptr<Type>> NarrowTypes(std::shared_ptr<Type>, std::shared_ptr<Type>);

} // namespace type
