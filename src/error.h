#pragma once

#include <string>

#include "token.h"

enum class errn
{
  Ok = 0,
};

class error
{
public:
  errn no;
  size_t file_id;
  std::string message;
  position pos;

  error(errn n, size_t fid, std::string m, position p) : no(n), file_id(fid), message(m), pos(p) {};
};
