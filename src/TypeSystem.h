#pragma once

#include <vector>

enum class TypeOfType
{
  Void,
  String,
  Integer,
  Function,
};

class Type
{
public:
  TypeOfType TypeOf;

  Type(TypeOfType type) : TypeOf(type) {};
  virtual ~Type() = default;

  virtual bool operator==(const Type &other) const;
  bool operator!=(const Type &other) const;
};

class TypeFunction : public Type
{
public:
  Type ReturnType;
  std::vector<Type> ParamTypes;

  TypeFunction(Type returnType, std::vector<Type> paramTypes)
      : Type(TypeOfType::Function), ReturnType(returnType), ParamTypes(paramTypes)
  {
  }
  ~TypeFunction() = default;

  bool operator==(const Type &other) const override;
};
