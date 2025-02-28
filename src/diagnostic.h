#pragma once

#include <cstddef>
#include <memory>
#include <string>

#include "error.h"
#include "loader.h"
#include "token.h"

enum class DiagnosticSeverity
{
  INFO = 1,
  WARN = 2,
  ERROR = 3,
};

class Diagnostic
{
public:
  Errno Errn;
  Position Pos;
  std::shared_ptr<Source> Sourc;
  DiagnosticSeverity Severity;
  std::string Message;

  Diagnostic(Errno errn, Position pos, std::shared_ptr<Source> source, DiagnosticSeverity severity, std::string message) : Errn(errn), Pos(pos), Sourc(source), Severity(severity), Message(message) {};
};

class DiagnosticEngine
{
public:
  DiagnosticEngine() {}

  void Report(Diagnostic diagnostic);

private:
  std::string Paint(std::string code, std::string color);
  std::string Highlight(std::string code, size_t start, size_t end, std::string color);

  std::string MatchSevevirtyString(DiagnosticSeverity severity);
  std::string MatchSeverityColor(DiagnosticSeverity severity);
};
