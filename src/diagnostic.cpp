#include <cmath>
#include <cstddef>
#include <format>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "diagnostic.h"
#include "token.h"

#define COLOR_RESET "\x1b[0m"
#define COLOR_RED_BOLD "\x1b[1;31m"
#define COLOR_RED_UNDERLINED "\x1b[4;31m"
#define COLOR_CYAN "\x1b[36m"
#define COLOR_BROW "\x1b[38;5;8m"

#define RESET "\033[0m"
#define BOLD "\033[1m"
#define RED "\033[31m"
#define YELLOW "\033[33m"
#define GREEN "\033[32m"
#define WHITE "\033[37m"

#define BOLD_RED BOLD RED
#define BOLD_YELLOW BOLD YELLOW
#define BOLD_GREEN BOLD GREEN
#define BOLD_WHITE BOLD WHITE

std::string diagnostic_engine::paint(std::string text, std::string colr)
{
  text.insert(0, colr);
  text.append(COLOR_RESET);
  return text;
}

struct LineInfo
{
  size_t start;
  size_t end;
};

std::vector<LineInfo> split_lines(const std::string &code)
{
  std::vector<LineInfo> lines;
  size_t line_start = 0;
  for (size_t i = 0; i < code.size(); ++i)
  {
    if (code[i] == '\n')
    {
      lines.push_back({line_start, i});
      line_start = i + 1;
    }
  }
  if (line_start < code.size())
  {
    lines.push_back({line_start, code.size()});
  }
  return lines;
}

size_t find_line_index(const std::vector<LineInfo> &lines, size_t pos)
{
  for (size_t i = 0; i < lines.size(); ++i)
  {
    if (pos >= lines[i].start && pos < lines[i].end)
    {
      return i;
    }
  }
  return lines.size(); // Not found
}

std::string diagnostic_engine::highlight(std::string code, size_t start, size_t end, std::string colr)
{
  // Clamp start and end to valid range
  end++;
  start = std::min(start, code.size());
  end = std::min(end, code.size());
  start = std::max(start, static_cast<size_t>(0));
  end = std::max(end, start);

  auto lines = split_lines(code);
  if (lines.empty())
  {
    return "";
  }

  // Find start_line and end_line
  size_t start_line = find_line_index(lines, start);
  if (start_line >= lines.size())
  {
    start_line = lines.size() - 1;
  }

  size_t end_line;
  if (end == 0)
  {
    end_line = start_line;
  }
  else
  {
    end_line = find_line_index(lines, end - 1);
    if (end_line >= lines.size())
    {
      end_line = lines.size() - 1;
    }
  }

  // Calculate context lines
  size_t context_start = (start_line >= 2) ? start_line - 2 : 0;
  size_t context_end = std::min(end_line + 2, lines.size() - 1);

  // Collect context lines
  std::vector<size_t> context_lines;
  for (size_t i = context_start; i <= context_end; ++i)
  {
    context_lines.push_back(i);
  }

  // Prepare output
  std::stringstream output;
  size_t max_line_number = context_end + 1;
  size_t line_number_width = std::to_string(max_line_number).size();

  for (size_t line_idx : context_lines)
  {
    const auto &line = lines[line_idx];
    std::string line_code = code.substr(line.start, line.end - line.start);
    size_t line_number = line_idx + 1;

    // Output code line
    output << std::setw(static_cast<int>(line_number_width)) << line_number << " | " << line_code << "\n";

    // Check if line is part of the highlighted region
    if (line.end <= start || line.start >= end)
    {
      continue;
    }

    // Calculate columns to highlight
    size_t highlight_start = std::max(start, line.start);
    size_t highlight_end = std::min(end, line.end);
    size_t start_col = highlight_start - line.start;
    size_t end_col = highlight_end - line.start;

    // Handle zero-length (caret at a position)
    if (start_col == end_col)
    {
      if (start_col < line_code.size())
      {
        end_col = start_col + 1;
      }
      else
      {
        continue; // No space to place caret
      }
    }

    // Generate caret line
    std::string caret_line(line_code.size(), ' ');
    for (size_t i = start_col; i < end_col && i < caret_line.size(); ++i)
    {
      caret_line[i] = '~';
    }

    // Output caret line
    output << std::setw(static_cast<int>(line_number_width)) << "" << " | " << colr << caret_line << RESET << "\n";
  }

  return output.str();
}

void diagnostic_engine::report(diagnostic diag)
{
  std::cerr << paint(std::format("{}:{}:{} ", diag.src.get()->path, diag.pos.line, diag.pos.col), BOLD_WHITE);
  std::cerr << paint(std::format("{}: {}", match_sevevirty_string(diag.severity), diag.message), match_severity_color(diag.severity)) << std::endl;
  std::cerr << std::endl;
  std::cerr << highlight(diag.src.get()->content, diag.pos.start, diag.pos.end, match_severity_color(diag.severity)) << std::endl;
}

std::string diagnostic_engine::match_sevevirty_string(diagnostic_severity severity)
{
  switch (severity)
  {
  case diagnostic_severity::error:
    return "ERROR";
  case diagnostic_severity::warn:
    return "WARNING";
  case diagnostic_severity::info:
    return "INFO";
  }
  return "UNKNOWN SEVERITY";
}

std::string diagnostic_engine::match_severity_color(diagnostic_severity severity)
{
  switch (severity)
  {
  case diagnostic_severity::error:
    return BOLD_RED;
  case diagnostic_severity::warn:
    return BOLD_YELLOW;
  case diagnostic_severity::info:
    return BOLD_GREEN;
  }
  return BOLD_WHITE;
}
