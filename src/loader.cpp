#include <filesystem>
#include <format>
#include <fstream>

#include "error.h"
#include "loader.h"
#include "result.h"

result<std::shared_ptr<source>, error> loader::load(std::string path)
{
  std::ifstream file(path);
  if (!file.is_open())
  {
    std::error_code ec;
    (void)std::filesystem::status(path, ec);

    return result<std::shared_ptr<source>, error>(error(errn::fs_error, std::format("Couldn't open file {}: {}", path, ec.message())));
  }
  std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  auto src = std::make_shared<source>(sources.size(), path, content);
  sources.push_back(src);
  return result<std::shared_ptr<source>, error>(src);
}

std::optional<std::shared_ptr<source>> loader::find_source(size_t id)
{
  if (id >= sources.size())
    return std::nullopt;
  return sources.at(id);
}
