#include "type.h"

namespace type
{

std::string InspectBase(Base base)
{
  switch (base)
  {
  case Base::VOID:
    return "void";
  case Base::STRING:
    return "string";
  case Base::I32:
    return "i32";
  case Base::F_STRING:
    return "format string";
  case Base::FUNCTION:
    return "function";
  case Base::ANY:
    return "any";
  case Base::OBJECT:
    return "object";
  }
  return "UNKNWON TYPE";
}

bool MatchBaseTypes(Base lhs, Base rhs)
{
  if (Base::ANY == lhs || Base::ANY == rhs)
  {
    return true;
  }

  if ((lhs == Base::STRING && rhs == Base::F_STRING) || (rhs == Base::STRING && lhs == Base::F_STRING))
  {
    return true;
  }

  return lhs == rhs;
}

std::optional<std::shared_ptr<Type>> NarrowTypes(std::shared_ptr<Type> lhs, std::shared_ptr<Type> rhs)
{
  (void)lhs;
  (void)rhs;

  return std::nullopt;
}

} // namespace type
