#include "TypeSystem.h"

#include <cstdlib>
#include <iostream>
#include <string>

std::string Type::ToString() const
{
  switch (Base)
  {
  case BaseType::Void:
    return "void";
  case BaseType::Integer:
    return "int";
  case BaseType::String:
    return "str";
  case BaseType::Function:
    return "fn";
  }

  std::cerr << "Unreachable\n";
  std::exit(1);
}

bool Type::operator==(const Type &other) const { return Base == other.Base; }
bool Type::operator!=(const Type &other) const { return !(*this == other); }

bool TypeFunction::operator==(const Type &other) const
{
  if (Base != other.Base)
  {
    return false;
  }

  const TypeFunction otherFnType = static_cast<const TypeFunction &>(other);

  if (ReturnType != otherFnType.ReturnType)
  {
    return false;
  }

  if (IsVarArgs != otherFnType.IsVarArgs)
  {
    return false;
  }

  if (ParamTypes.size() != otherFnType.ParamTypes.size())
  {
    return false;
  }

  for (const Type &param : ParamTypes)
  {
    for (const Type &otherParam : otherFnType.ParamTypes)
      if (param != otherParam)
      {
        return false;
      }
  }

  return true;
}
