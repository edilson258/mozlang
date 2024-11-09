#include "DiagnosticEngine.h"
#include "Painter.h"

#include <cstdlib>
#include <iostream>
#include <string>

void DiagnosticEngine::Error(ErrorCode code, std::string message, Location loc)
{
  std::cerr << Painter::Paint(FilePath, Color::Cyan) << ":" << loc.Line << ":" << loc.Column << " ";
  std::cerr << Painter::Paint("ERROR", Color::RedBold) << " ";
  std::cerr << Painter::Paint("MZE" + std::to_string(static_cast<int>(code)) + ":", Color::Brown) << " ";
  std::cerr << message << std::endl << std::endl;
  std::cerr << Painter::Highlight(FileContent, loc.Begin, loc.End) << std::endl;
}
