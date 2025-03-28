#include "context.h"

void ModuleContext::Save(std::string name, Ptr<Bind> bind)
{
  Store[name] = bind;
}

Ptr<Bind> ModuleContext::Get(std::string key)
{
  if (Store.find(key) == Store.end())
  {
    return nullptr;
  }
  return Store[key];
}
