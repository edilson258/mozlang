#include "DiagnosticEngine.h"
#include "Painter.h"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

void DiagnosticEngine::Error(ErrorCode code, std::string message, Location loc)
{
  std::cerr << HandleErr(code, message, loc);
}

void DiagnosticEngine::PushErr(ErrorCode code, std::string message, Location loc)
{
  Errors.push_back(HandleErr(code, message, loc));
}

std::string DiagnosticEngine::HandleErr(ErrorCode code, std::string message, Location loc)
{
  std::ostringstream oss;
  oss << Painter::Paint(FilePath, Color::Cyan) << ":" << loc.Line << ":" << loc.Column << " ";
  oss << Painter::Paint("ERROR", Color::RedBold) << " ";
  oss << Painter::Paint("MZE" + std::to_string(static_cast<int>(code)) + ":", Color::Brown) << " ";
  oss << message << std::endl << std::endl;
  oss << Painter::Highlight(FileContent, loc.Begin, loc.End) << std::endl;
  return oss.str();
}
