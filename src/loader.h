#pragma once

#include <cstddef>
#include <map>
#include <memory>
#include <string>

#include "error.h"
#include "result.h"

using ModuleID = size_t;

class Module
{
public:
  std::string m_Path;
  std::string m_Content;

  Module(std::string path, std::string content) : m_Path(path), m_Content(content) {};
};

class ModuleManager
{
public:
  std::map<ModuleID, std::shared_ptr<Module>> m_Modules;

  ModuleManager() : m_Modules() {};

  Result<ModuleID, Error> Load(std::string);
};
