#pragma once

#include <string>

enum Color
{
  Brown = 1,
  Cyan,
  RedBold,
};

class Painter
{
public:
  static std::string Paint(std::string text, Color color);
  static std::string Highlight(const std::string &code, unsigned long begin, unsigned long end);
};
