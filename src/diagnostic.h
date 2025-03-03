#pragma once

#include <cstddef>
#include <memory>
#include <optional>
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

class DiagnosticReference
{
public:
  Errno m_Errno;
  size_t m_FileID;
  Position m_Position;
  std::string m_Message;

  DiagnosticReference(Errno errn, size_t fileID, Position position, std::string message) : m_Errno(errn), m_FileID(fileID), m_Position(position), m_Message(message) {};
};

class Diagnostic
{
public:
  Errno m_Errno;
  Position m_Position;
  std::shared_ptr<Source> m_Source;
  DiagnosticSeverity m_Severity;
  std::string m_Message;
  std::optional<DiagnosticReference> m_Reference;

  Diagnostic(Errno errn, Position pos, std::shared_ptr<Source> source, DiagnosticSeverity severity, std::string message, std::optional<DiagnosticReference> reference = std::nullopt) : m_Errno(errn), m_Position(pos), m_Source(source), m_Severity(severity), m_Message(message), m_Reference(reference) {};
};

class DiagnosticEngine
{
public:
  DiagnosticEngine() {}

  static void Report(Diagnostic diagnostic);

private:
  static std::string Paint(std::string code, std::string color);
  static std::string Highlight(std::string code, size_t start, size_t end, std::string color);

  static std::string MatchSeverityColor(DiagnosticSeverity severity);
  static std::string MatchSevevirtyString(DiagnosticSeverity severity);
};
