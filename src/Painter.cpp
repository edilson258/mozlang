#include "Painter.h"

#include <iostream>
#include <sstream>
#include <string>

#define COLOR_RESET "\x1b[0m"
#define COLOR_RED_BOLD "\x1b[1;31m"
#define COLOR_RED_UNDERLINED "\x1b[4;31m"
#define COLOR_CYAN "\x1b[36m"
#define COLOR_BROW "\x1b[38;5;8m"

std::string Painter::Paint(std::string text, Color colorKind)
{
  switch (colorKind)
  {
  case Color::RedBold:
    text.insert(0, COLOR_RED_BOLD);
    break;
  case Color::Cyan:
    text.insert(0, COLOR_CYAN);
    break;
  case Color::Brown:
    text.insert(0, COLOR_BROW);
    break;
  }
  text.append(COLOR_RESET);

  return text;
}

std::string Painter::Highlight(const std::string &code, unsigned long begin, unsigned long end)
{
  size_t codeLen = code.length();
  end            = end >= codeLen ? codeLen - 1 : end;

  size_t cursor                    = 0;
  size_t lineCounter               = 1;
  size_t beginOfLine               = 0;
  size_t indexWhereSliceBegin      = 0;
  size_t lineNumberWhereSliceBegin = 0;
  size_t lineNumberWhereSliceEnd   = 0;

  while (cursor < codeLen)
  {
    char currentChar = code[cursor];

    if (cursor == begin)
    {
      lineNumberWhereSliceBegin = lineCounter;
      indexWhereSliceBegin      = beginOfLine;
    }

    if (currentChar == '\n')
    {
      lineCounter += 1;
      beginOfLine = cursor + 1; // +1 to skipe '\n'

      if (cursor >= end)
      {
        lineNumberWhereSliceEnd = lineCounter;
        break;
      }
    }

    cursor++;
  }

  std::ostringstream oss;

  unsigned long xs = std::to_string(lineNumberWhereSliceEnd).length();
  oss << std::string(3 + xs, ' ') + "|\n";

  cursor = indexWhereSliceBegin;
  for (size_t i = lineNumberWhereSliceBegin; i < lineNumberWhereSliceEnd; ++i)
  {
    oss << "  " << i << " | ";

    while (code[cursor] != '\n')
    {
      if (cursor >= begin && cursor <= end)
      {
        oss << COLOR_RED_UNDERLINED << code[cursor] << COLOR_RESET;
      }
      else
      {
        oss << code[cursor];
      }
      cursor++;
    }

    oss << code[cursor++];
  }

  oss << std::string(3 + xs, ' ') + "|\n";

  return oss.str();
}
