#pragma once

#include <map>
#include <string>

#include "error.h"
#include "result.h"

#include <cstddef>
#include <string>
#include <vector>

#include "ast.h"

using ModuleID = size_t;

enum class ModuleStatus
{
  IDLE = 1,
  LOADED,
  INVALID,
};

class Module
{
public:
  ModuleID m_ID;
  ModuleStatus m_Status;
  std::string m_Path;
  std::string m_Content;
  Ptr<Ast> m_AST;
  Ptr<class ModuleContext> m_Exports;
  std::vector<ModuleID> m_Imports;

  Module(ModuleID id, std::string path, std::string content) : m_ID(id), m_Status(ModuleStatus::IDLE), m_Path(path), m_Content(content), m_AST(nullptr), m_Exports(nullptr), m_Imports() {};
};

class ModuleManager
{
public:
  std::map<ModuleID, Ptr<Module>> m_Modules;
  std::map<std::string, ModuleID> m_PathToID;

  ModuleManager() : m_Modules() {};

  Result<Ptr<Module>, Error> Load(std::string);
};
