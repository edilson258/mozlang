#include <filesystem>
#include <format>
#include <fstream>

#include "error.h"
#include "loader.h"
#include "result.h"

Result<std::shared_ptr<Source>, Error> Loader::Load(std::string path)
{
  std::ifstream file(path);
  if (!file.is_open())
  {
    std::error_code errorCode;
    (void)std::filesystem::status(path, errorCode);

    return Result<std::shared_ptr<Source>, Error>(Error(Errno::FS_ERROR, std::format("Couldn't open file {}: {}", path, errorCode.message())));
  }
  std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  auto src = std::make_shared<Source>(m_Sources.size(), path, content);
  m_Sources.push_back(src);
  return Result<std::shared_ptr<Source>, Error>(src);
}

std::optional<std::shared_ptr<Source>> Loader::FindSource(size_t id)
{
  if (id >= m_Sources.size())
    return std::nullopt;
  return m_Sources.at(id);
}
