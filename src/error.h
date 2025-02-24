#pragma once

#include <string>

enum class errn
{
  ok = 0,
  name_error,
  fs_error,
};

class error
{
public:
  errn no;
  std::string message;

  error(errn n, std::string m) : no(n), message(m) {};
};
