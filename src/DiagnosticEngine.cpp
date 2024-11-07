#include "DiagnosticEngine.h"
#include "Painter.h"
#include "Token.h"

#include <cstdlib>
#include <iostream>
#include <string>

void DiagnosticEngine::ErrorAndDie(const std::filesystem::path &filePath, const std::string &fileContent,
                                   ErrorCode code, std::string message, TokenSpan span)
{
  std::cerr << Painter::Paint(filePath, Color::Cyan) << ":" << span.Line << ":" << span.Column << " ";
  std::cerr << Painter::Paint("ERROR", Color::RedBold) << " ";
  std::cerr << Painter::Paint("MZE" + std::to_string(static_cast<int>(code)) + ":", Color::Brown) << " ";
  std::cerr << message << std::endl << std::endl;
  std::cerr << Painter::Highlight(fileContent, span.RangeBegin, span.RangeEnd) << std::endl;
  std::exit(1);
}
