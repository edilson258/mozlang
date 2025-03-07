#pragma once

#include <cstddef>
#include <optional>
#include <string>

#include "error.h"
#include "module.h"
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
  ModuleID m_ModuleID;
  Position m_Position;
  std::string m_Message;

  DiagnosticReference(Errno errn, ModuleID moduleID, Position position, std::string message) : m_Errno(errn), m_ModuleID(moduleID), m_Position(position), m_Message(message) {};
};

class Diagnostic
{
public:
  Errno m_Errno;
  Position m_Position;
  ModuleID m_ModuleID;
  DiagnosticSeverity m_Severity;
  std::string m_Message;
  std::optional<DiagnosticReference> m_Reference;

  Diagnostic(Errno errn, Position pos, ModuleID moduleID, DiagnosticSeverity severity, std::string message, std::optional<DiagnosticReference> reference = std::nullopt) : m_Errno(errn), m_Position(pos), m_ModuleID(moduleID), m_Severity(severity), m_Message(message), m_Reference(reference) {};
};

class DiagnosticEngine
{
public:
  DiagnosticEngine(ModuleManager &modManager) : m_ModManager(modManager) {}

  void Report(Diagnostic diagnostic);

private:
  ModuleManager &m_ModManager;

  std::string Paint(std::string code, std::string color);
  std::string Highlight(std::string code, size_t start, size_t end, std::string color);

  std::string MatchSeverityColor(DiagnosticSeverity severity);
  std::string MatchSevevirtyString(DiagnosticSeverity severity);
};
