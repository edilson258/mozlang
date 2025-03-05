#include <filesystem>
#include <fstream>
#include <memory>

#include "error.h"
#include "loader.h"
#include "result.h"

Result<ModuleID, Error> ModuleManager::Load(std::string path)
{
  std::ifstream file(path);
  if (!file.is_open())
  {
    std::error_code errorCode;
    (void)std::filesystem::status(path, errorCode);
    return Result<ModuleID, Error>(Error(Errno::FS_ERROR, errorCode.message()));
  }
  std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  ModuleID id = m_Modules.size();
  m_Modules[id] = std::make_shared<Module>(Module(path, content));
  return Result<ModuleID, Error>(id);
}
