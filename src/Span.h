#pragma once

class Span
{
public:
  unsigned long Line;
  unsigned long Column;
  unsigned long RangeBegin;
  unsigned long RangeEnd;

  Span() = default;
  Span(unsigned long line, unsigned long column, unsigned long begin, unsigned long end)
      : Line(line), Column(column), RangeBegin(begin), RangeEnd(end)
  {
  }

  bool operator==(const Span &other) const
  {
    if (Line == other.Line && Column == other.Column && RangeBegin == other.RangeBegin && RangeEnd == other.RangeEnd)
    {
      return true;
    }
    return false;
  }
};
