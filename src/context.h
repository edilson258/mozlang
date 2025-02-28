#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include "token.h"
#include "type.h"

enum class BindType
{
  LITERAL = 1,
  FUNCTION,
  PARAMETER,
};

class Binding
{
public:
  BindType m_BindType;
  size_t m_FileID;
  Position m_Position;
  std::shared_ptr<type::Type> m_Type;
  bool m_IsUsed;

  Binding(BindType bindType, std::shared_ptr<type::Type> type, size_t fileID, Position pos, bool isUsed = false) : m_BindType(bindType), m_FileID(fileID), m_Position(pos), m_Type(type), m_IsUsed(isUsed) {}
};

class BindingFunction : public Binding
{
public:
  Position NamePosition;
  Position ParamsPosition;

  BindingFunction(Position position, Position namePosition, Position paramsPosition, std::shared_ptr<type::Function> functionType, size_t fileID, bool used = false) : Binding(BindType::FUNCTION, functionType, fileID, position, used), NamePosition(namePosition), ParamsPosition(paramsPosition) {};
};

class Context
{
public:
  std::unordered_map<std::string, std::shared_ptr<Binding>> Store;

  Context() : Store() {};

  void Save(std::string name, std::shared_ptr<Binding> bind);
  std::optional<std::shared_ptr<Binding>> Get(std::string);
};
