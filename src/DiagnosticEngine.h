#pragma once

#include "Token.h"

#include <filesystem>
#include <string>

enum class ErrorCode
{
  UnquotedStringLiteral = 1001,
  UnknownToken          = 1002,
};

class DiagnosticEngine
{
public:
  const std::filesystem::path &FilePath;
  const std::string &FileContent;

  static void ErrorAndDie(const std::filesystem::path &, const std::string &, ErrorCode, std::string, TokenSpan);
};
