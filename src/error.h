#pragma once

#include <string>

enum class Errno
{
  OK = 1,
  FS_ERROR,
  NAME_ERROR,
  TYPE_ERROR,
  SYNTAX_ERROR,

  UNUSED_VALUE,
  DEAD_CODE,
};

class Error
{
public:
  Errno Errn;
  std::string Message;

  Error(Errno errn, std::string message) : Errn(errn), Message(message) {};
};
