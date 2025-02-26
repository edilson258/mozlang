#pragma once

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include "token.h"
#include "type.h"

enum class BindingOrigin
{
  LITERAL,
  RETURN_VALUE,
  BUILTIN,
};

class Binding
{
public:
  std::shared_ptr<Type> Typ;
  Position Pos;
  BindingOrigin Origin;
  bool Used;

  Binding(std::shared_ptr<Type> type, Position pos, BindingOrigin origin, bool used = false) : Typ(type), Pos(pos), Origin(origin), Used(used) {}

  static std::shared_ptr<Binding> make_string_literal(Position pos)
  {
    return std::make_shared<Binding>(std::make_shared<Type>(BaseType::STRING), pos, BindingOrigin::LITERAL);
  }
};

class Context
{
public:
  std::unordered_map<std::string, std::shared_ptr<Binding>> Store;

  Context() : Store() {};

  void Save(std::string name, std::shared_ptr<Binding> bind);
  std::optional<std::shared_ptr<Binding>> Get(std::string);
};
