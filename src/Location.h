#pragma once

class Location
{
public:
  unsigned long Line;
  unsigned long Column;
  unsigned long Begin;
  unsigned long End;

  Location() = default;
  Location(unsigned long line, unsigned long column, unsigned long begin, unsigned long end)
      : Line(line), Column(column), Begin(begin), End(end)
  {
  }

  bool operator==(const Location &other) const
  {
    if (Line == other.Line && Column == other.Column && Begin == other.Begin && End == other.End)
    {
      return true;
    }
    return false;
  }
};
