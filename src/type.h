#pragma once

#include <cstddef>
#include <memory>
#include <vector>

enum class BaseType
{
  STRING,
  FUNCTION,

  // internal
  NONE,
  F_STRING,
};

class Type
{
public:
  BaseType Base;

  Type(BaseType base) : Base(base) {}
};

class FunctionType : public Type
{
public:
  size_t RequiredArgumentsCount;
  std::vector<std::shared_ptr<Type>> Arguments;
  bool IsVariadicArguments;

  FunctionType(size_t requiredArgsCount, std::vector<std::shared_ptr<Type>> arguments, bool isVariadic = false) : Type(BaseType::FUNCTION), RequiredArgumentsCount(requiredArgsCount), Arguments(std::move(arguments)), IsVariadicArguments(isVariadic) {}
};
