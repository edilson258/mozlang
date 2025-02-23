#pragma once

#include <cstddef>
#include <filesystem>
#include <map>
#include <memory>
#include <string>

class source
{
public:
  size_t id;
  std::filesystem::path path;
  std::string content;

  source(size_t i, std::filesystem::path p, std::string c) : id(i), path(p), content(c) {};
};

class loader
{
public:
  loader() : sources() {};

  size_t load(std::string path);

private:
  std::map<size_t, std::shared_ptr<source>> sources;
};
