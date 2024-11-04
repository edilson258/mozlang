#include "TypeSystem.h"

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
