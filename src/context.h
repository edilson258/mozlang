#pragma once

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include "loader.h"
#include "token.h"
#include "type.h"

enum class BindType
{
  EXPRESSION = 1,
  FUNCTION,
  VARIABLE,
  PARAMETER,
  RETURN_VALUE,
  MODULE,
};

class Binding
{
public:
  BindType m_BindType;
  ModuleID m_ModuleID;
  Position m_Position;
  std::shared_ptr<type::Type> m_Type;
  bool m_IsUsed;

  Binding(BindType bindType, std::shared_ptr<type::Type> type, ModuleID moduleID, Position pos, bool isUsed = false) : m_BindType(bindType), m_ModuleID(moduleID), m_Position(pos), m_Type(type), m_IsUsed(isUsed) {}
};

class BindingFunction : public Binding
{
public:
  Position NamePosition;
  Position ParamsPosition;

  BindingFunction(Position position, Position namePosition, Position paramsPosition, std::shared_ptr<type::Function> functionType, ModuleID moduleID, bool used = false) : Binding(BindType::FUNCTION, functionType, moduleID, position, used), NamePosition(namePosition), ParamsPosition(paramsPosition) {};
};

class BindingModule : public Binding
{
public:
  Position m_AliasPosition;
  std::shared_ptr<class Context> m_Context;

  BindingModule(Position position, Position aliasPosition, ModuleID moduleID, std::shared_ptr<class Context> context, std::shared_ptr<type::Object> objectType) : Binding(BindType::MODULE, objectType, moduleID, position), m_AliasPosition(aliasPosition), m_Context(context) {}
};

class Context
{
public:
  std::unordered_map<std::string, std::shared_ptr<Binding>> Store;

  Context() : Store() {};

  void Save(std::string name, std::shared_ptr<Binding> bind);
  std::optional<std::shared_ptr<Binding>> Get(std::string);
};
