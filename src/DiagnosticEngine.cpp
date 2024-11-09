#include "DiagnosticEngine.h"
#include "Painter.h"

#include <cstdlib>
#include <iostream>
#include <string>

void DiagnosticEngine::Error(ErrorCode code, std::string message, Span span)
{
  std::cerr << Painter::Paint(FilePath, Color::Cyan) << ":" << span.Line << ":" << span.Column << " ";
  std::cerr << Painter::Paint("ERROR", Color::RedBold) << " ";
  std::cerr << Painter::Paint("MZE" + std::to_string(static_cast<int>(code)) + ":", Color::Brown) << " ";
  std::cerr << message << std::endl << std::endl;
  std::cerr << Painter::Highlight(FileContent, span.RangeBegin, span.RangeEnd) << std::endl;
}
