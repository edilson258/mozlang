#include "TypeSystem.h"

bool Type::operator==(const Type &other) const { return TypeOf == other.TypeOf; }
bool Type::operator!=(const Type &other) const { return !(*this == other); }

bool TypeFunction::operator==(const Type &other) const
{
  if (TypeOfType::Function != other.TypeOf)
  {
    return false;
  }

  const TypeFunction otherFnType = static_cast<const TypeFunction &>(other);

  if (ReturnType != otherFnType.ReturnType)
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
