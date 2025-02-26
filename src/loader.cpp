#include <filesystem>
#include <format>
#include <fstream>

#include "error.h"
#include "loader.h"
#include "result.h"

result<std::shared_ptr<Source>, ERROR> Loader::Load(std::string path)
{
  std::ifstream file(path);
  if (!file.is_open())
  {
    std::error_code errorCode;
    (void)std::filesystem::status(path, errorCode);

    return result<std::shared_ptr<Source>, ERROR>(ERROR(Errno::FS_ERROR, std::format("Couldn't open file {}: {}", path, errorCode.message())));
  }
  std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  auto src = std::make_shared<Source>(Sources.size(), path, content);
  Sources.push_back(src);
  return result<std::shared_ptr<Source>, ERROR>(src);
}

std::optional<std::shared_ptr<Source>> Loader::FindSource(size_t id)
{
  if (id >= Sources.size())
    return std::nullopt;
  return Sources.at(id);
}
