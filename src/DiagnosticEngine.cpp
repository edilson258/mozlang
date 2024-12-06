#include "DiagnosticEngine.h"
#include "Painter.h"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

[[noreturn]] void DiagnosticEngine::ErrorAndExit(ErrorCode code, std::string message, Location loc)
{
  std::cerr << HandleError(code, message, loc);
  std::exit(1);
}

void DiagnosticEngine::PushError(ErrorCode code, std::string message, Location loc)
{
  Errors.push_back(HandleError(code, message, loc));
}

std::string DiagnosticEngine::HandleError(ErrorCode code, std::string message, Location loc)
{
  std::ostringstream oss;
  oss << Painter::Paint(FilePath, Color::Cyan) << ":" << loc.Line << ":" << loc.Column << " ";
  oss << Painter::Paint("ERROR", Color::RedBold) << " ";
  oss << Painter::Paint("MZE" + std::to_string(static_cast<int>(code)) + ":", Color::Brown) << " ";
  oss << message << std::endl << std::endl;
  oss << Painter::Highlight(FileContent, loc.Begin, loc.End) << std::endl;
  return oss.str();
}
