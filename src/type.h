#pragma once

#include <cstddef>
#include <memory>
#include <vector>

namespace type
{
enum class Base
{
  STRING,
  FUNCTION,

  // internal
  ANY,
  F_STRING,
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
  bool m_IsVariadicArguments;

  Function(size_t reqArgsCount, std::vector<std::shared_ptr<Type>> arguments, bool isVariadic = false) : Type(Base::FUNCTION), m_ReqArgsCount(reqArgsCount), m_Arguments(std::move(arguments)), m_IsVariadicArguments(isVariadic) {}
};

} // namespace type
