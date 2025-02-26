#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include "token.h"
#include "type.h"

enum class BindingType
{
  LITERAL = 1,
  FUNCTION,
  PARAMETER,
};

class Binding
{
public:
  BindingType Typ;
  Position Pos;
  size_t FileID;
  bool Used;

protected:
  Binding(BindingType type, Position pos, size_t fileID, bool used = false) : Typ(type), Pos(pos), FileID(fileID), Used(used) {}
};

class BindingLiteral : public Binding
{
public:
  BaseType BaseTyp;

  BindingLiteral(Position pos, BaseType baseTyp, size_t fileID, bool used = false) : Binding(BindingType::LITERAL, pos, fileID, used), BaseTyp(baseTyp) {}
};

class BindingParameter : public Binding
{
public:
  std::shared_ptr<Type> Typ;

  BindingParameter(Position pos, std::shared_ptr<Type> typ, size_t fileID, bool used = false) : Binding(BindingType::PARAMETER, pos, fileID, used), Typ(typ) {}
};

class BindingFunction : public Binding
{
public:
  Position NamePosition;
  Position ParamsPosition;
  std::shared_ptr<FunctionType> FnType;

  BindingFunction(Position position, Position namePosition, Position paramsPosition, std::shared_ptr<FunctionType> functionType, size_t fileID, bool used = false) : Binding(BindingType::FUNCTION, position, fileID, used), NamePosition(namePosition), ParamsPosition(paramsPosition), FnType(functionType) {};
};

class Context
{
public:
  std::unordered_map<std::string, std::shared_ptr<Binding>> Store;

  Context() : Store() {};

  void Save(std::string name, std::shared_ptr<Binding> bind);
  std::optional<std::shared_ptr<Binding>> Get(std::string);
};
