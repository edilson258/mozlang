#include <cmath>
#include <cstddef>
#include <format>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "diagnostic.h"

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

std::string DiagnosticEngine::Paint(std::string text, std::string color)
{
  text.insert(0, color);
  text.append(RESET);
  return text;
}

struct LineInfo
{
  size_t Start;
  size_t End;
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
    if (pos >= lines[i].Start && pos < lines[i].End)
    {
      return i;
    }
  }
  return lines.size(); // Not found
}

std::string DiagnosticEngine::Highlight(std::string code, size_t start, size_t end, std::string colr)
{
  end++;
  // Clamp start and end to valid range
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
  size_t startLine = find_line_index(lines, start);
  if (startLine >= lines.size())
  {
    startLine = lines.size() - 1;
  }

  size_t endLine;
  if (end == 0)
  {
    endLine = startLine;
  }
  else
  {
    endLine = find_line_index(lines, end - 1);
    if (endLine >= lines.size())
    {
      endLine = lines.size() - 1;
    }
  }

  // Calculate context lines
  size_t contextStart = (startLine >= 2) ? startLine - 2 : 0;
  size_t contextEnd = std::min(endLine + 2, lines.size() - 1);

  // Collect context lines
  std::vector<size_t> contextLines;
  for (size_t i = contextStart; i <= contextEnd; ++i)
  {
    contextLines.push_back(i);
  }

  // Prepare output
  std::stringstream output;
  size_t max_line_number = contextEnd + 1;
  size_t line_number_width = std::to_string(max_line_number).size();

  for (size_t lineIndex : contextLines)
  {
    const auto &line = lines[lineIndex];
    std::string lineCode = code.substr(line.Start, line.End - line.Start);
    size_t lineNumber = lineIndex + 1;

    // Output code line
    output << std::setw(static_cast<int>(line_number_width)) << lineNumber << " | " << lineCode << "\n";

    // Check if line is part of the highlighted region
    if (line.End <= start || line.Start >= end)
    {
      continue;
    }

    // Calculate columns to highlight
    size_t highlightStart = std::max(start, line.Start);
    size_t highlightEnd = std::min(end, line.End);
    size_t startColumn = highlightStart - line.Start;
    size_t endColumn = highlightEnd - line.Start;

    // Handle zero-length (caret at a position)
    if (startColumn == endColumn)
    {
      if (startColumn < lineCode.size())
      {
        endColumn = startColumn + 1;
      }
      else
      {
        continue; // No space to place caret
      }
    }

    // Generate caret line
    std::string caretLine(lineCode.size(), ' ');
    for (size_t i = startColumn; i < endColumn && i < caretLine.size(); ++i)
    {
      caretLine[i] = '^';
    }

    // Output caret line
    output << std::setw(static_cast<int>(line_number_width)) << "" << " | " << colr << caretLine << RESET << "\n";
  }

  return output.str();
}

std::string insertTabAfterNewline(const std::string &input)
{
  std::string result = input;
  size_t pos = 0;
  while ((pos = result.find("\n", pos)) != std::string::npos)
  {
    result.insert(pos + 1, "\t");
    pos += 2;
  }
  return result;
}

void DiagnosticEngine::Report(Diagnostic diagnostic)
{
  std::cerr << Paint(std::format("{}:{}:{} ", m_ModManager.m_Modules[diagnostic.m_ModuleID].get()->m_Path, diagnostic.m_Position.m_Line, diagnostic.m_Position.m_Column), BOLD_WHITE);
  std::cerr << Paint(std::format("{}: {}", MatchSevevirtyString(diagnostic.m_Severity), diagnostic.m_Message), MatchSeverityColor(diagnostic.m_Severity)) << std::endl;
  std::cerr << std::endl;
  std::cerr << Highlight(m_ModManager.m_Modules[diagnostic.m_ModuleID].get()->m_Content, diagnostic.m_Position.m_Start, diagnostic.m_Position.m_End, MatchSeverityColor(diagnostic.m_Severity)) << std::endl;

  if (diagnostic.m_Reference.has_value())
  {
    auto ref = diagnostic.m_Reference.value();
    std::cerr << std::format("{}:{}:{} {}", m_ModManager.m_Modules[ref.m_ModuleID].get()->m_Path, ref.m_Position.m_Line, ref.m_Position.m_Column, ref.m_Message) << std::endl;
    std::cerr << "\t" << std::endl;
    std::cerr << "\t" << insertTabAfterNewline(Highlight(m_ModManager.m_Modules[ref.m_ModuleID].get()->m_Content, ref.m_Position.m_Start, ref.m_Position.m_End, MatchSeverityColor(DiagnosticSeverity::INFO))) << std::endl;
  }
}

std::string DiagnosticEngine::MatchSevevirtyString(DiagnosticSeverity severity)
{
  switch (severity)
  {
  case DiagnosticSeverity::ERROR:
    return "ERROR";
  case DiagnosticSeverity::WARN:
    return "WARN";
  case DiagnosticSeverity::INFO:
    return "INFO";
  }
  return "UNKNOWN SEVERITY";
}

std::string DiagnosticEngine::MatchSeverityColor(DiagnosticSeverity severity)
{
  switch (severity)
  {
  case DiagnosticSeverity::ERROR:
    return BOLD_RED;
  case DiagnosticSeverity::WARN:
    return BOLD_YELLOW;
  case DiagnosticSeverity::INFO:
    return BOLD_GREEN;
  }
  return BOLD_WHITE;
}
