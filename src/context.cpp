#include "context.h"

void Context::Save(std::string name, std::shared_ptr<Binding> bind)
{
  Store[name] = bind;
}

std::optional<std::shared_ptr<Binding>> Context::Get(std::string key)
{
  if (Store.find(key) == Store.end())
  {
    return std::nullopt;
  }
  return Store[key];
}
