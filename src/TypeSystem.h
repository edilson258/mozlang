#pragma once

#include <string>
#include <vector>

enum class BaseType
{
  Void,
  String,
  Integer,
  Boolean,
  Function,
};

class Type
{
public:
  BaseType Base;

  Type(BaseType base) : Base(base) {};
  virtual ~Type() = default;

  virtual bool operator==(const Type &other) const;
  bool operator!=(const Type &other) const;

  std::string ToString() const;
};

class TypeFunction : public Type
{
public:
  Type ReturnType;
  std::vector<Type> ParamTypes;
  bool IsVarArgs;

  TypeFunction(Type returnType, std::vector<Type> paramTypes, bool isVarArgs)
      : Type(BaseType::Function), ReturnType(returnType), ParamTypes(paramTypes), IsVarArgs(isVarArgs)
  {
  }
  ~TypeFunction() = default;

  bool operator==(const Type &other) const override;
};
