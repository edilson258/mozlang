#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "error.h"
#include "result.h"

class source
{
public:
  size_t id;
  std::string path;
  std::string content;

  source(size_t i, std::string p, std::string c) : id(i), path(p), content(c) {};
};

class loader
{
public:
  loader() : sources() {};

  result<std::shared_ptr<source>, error> load(std::string path);
  std::optional<std::shared_ptr<source>> find_source(size_t id);

private:
  std::vector<std::shared_ptr<source>> sources;
};
