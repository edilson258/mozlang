#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "error.h"
#include "result.h"

class Source
{
public:
  size_t m_ID;
  std::string m_Path;
  std::string m_Content;

  Source(size_t id, std::string path, std::string content) : m_ID(id), m_Path(path), m_Content(content) {};
};

class Loader
{
public:
  Loader() : m_Sources() {};

  Result<std::shared_ptr<Source>, Error> Load(std::string path);
  std::optional<std::shared_ptr<Source>> FindSource(size_t id);

private:
  std::vector<std::shared_ptr<Source>> m_Sources;
};
