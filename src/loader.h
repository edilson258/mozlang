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
  size_t id;
  std::string path;
  std::string content;

  Source(size_t i, std::string p, std::string c) : id(i), path(p), content(c) {};
};

class Loader
{
public:
  Loader() : Sources() {};

  result<std::shared_ptr<Source>, ERROR> Load(std::string path);
  std::optional<std::shared_ptr<Source>> FindSource(size_t id);

private:
  std::vector<std::shared_ptr<Source>> Sources;
};
