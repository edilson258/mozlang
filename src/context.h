#pragma once

#include <map>
#include <string>

#include "module.h"
#include "pointer.h"
#include "token.h"
#include "type.h"

enum class BindT
{
  Expr,
  Fun,
  Var,
  Param,
  RetVal,
  Mod,
};

class Bind
{
public:
  BindT m_BindT;
  ModuleID m_ModID;
  Position m_Pos;
  Ptr<type::Type> m_Type;
  bool m_IsUsed;
  bool m_IsPub;
  Ptr<Bind> m_Ref;

  Bind(BindT bindT, Ptr<type::Type> type, ModuleID modID, Position pos, bool isUsed = false, bool isPub = false, Ptr<Bind> ref = nullptr) : m_BindT(bindT), m_ModID(modID), m_Pos(pos), m_Type(type), m_IsUsed(isUsed), m_IsPub(isPub), m_Ref(ref) {}
};

class BindFun : public Bind
{
public:
  Position NamePosition;
  Position ParamsPosition;

  BindFun(Position position, Position namePosition, Position paramsPosition, Ptr<type::Function> funType, ModuleID moduleID, bool used = false, bool isPublic = false) : Bind(BindT::Fun, funType, moduleID, position, used, isPublic), NamePosition(namePosition), ParamsPosition(paramsPosition) {};
};

class BindMod : public Bind
{
public:
  std::string m_Name;
  Position m_NamePos;
  Ptr<class ModuleContext> m_Context;

  BindMod(std::string name, Position position, Position aliasPosition, ModuleID moduleID, Ptr<class ModuleContext> context, Ptr<type::Object> objT) : Bind(BindT::Mod, objT, moduleID, position), m_Name(name), m_NamePos(aliasPosition), m_Context(context) {}
};

class ModuleContext
{
public:
  std::map<std::string, Ptr<Bind>> Store;

  ModuleContext() : Store() {};

  void Save(std::string name, Ptr<Bind> bind);
  Ptr<Bind> Get(std::string);
};
