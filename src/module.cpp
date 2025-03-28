#include <filesystem>
#include <fstream>

#include "error.h"
#include "module.h"
#include "pointer.h"
#include "result.h"

Result<Ptr<Module>, Error> ModuleManager::Load(std::string path)
{
  if (m_PathToID.find(path) != m_PathToID.end())
  {
    return m_Modules.at(m_PathToID.at(path));
  }
  std::ifstream file(path);
  if (!file.is_open())
  {
    std::error_code errorCode;
    (void)std::filesystem::status(path, errorCode);
    return Error(Errno::FS_ERROR, errorCode.message());
  }
  std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  ModuleID id = m_Modules.size();
  auto module = MakePtr(Module(id, path, content));
  m_Modules[id] = module;
  return module;
}
