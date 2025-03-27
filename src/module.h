#pragma once

#include <map>
#include <memory>
#include <string>

#include "error.h"
#include "result.h"

#include <cstddef>
#include <memory>
#include <optional>
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
  std::optional<std::shared_ptr<AST>> m_AST;
  std::optional<std::shared_ptr<class ModuleContext>> m_Exports;
  std::vector<ModuleID> m_ImportedModules;

  Module(ModuleID id, std::string path, std::string content) : m_ID(id), m_Status(ModuleStatus::IDLE), m_Path(path), m_Content(content), m_AST(), m_Exports(), m_ImportedModules() {};
};

class ModuleManager
{
public:
  std::map<ModuleID, std::shared_ptr<Module>> m_Modules;
  std::map<std::string, ModuleID> m_PathToID;

  ModuleManager() : m_Modules() {};

  Result<std::shared_ptr<Module>, Error> Load(std::string);
};
