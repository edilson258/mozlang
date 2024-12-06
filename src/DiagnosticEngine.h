#pragma once

#include "Location.h"

#include <filesystem>
#include <string>
#include <vector>

enum class ErrorCode
{
  UnquotedString        = 1001,
  UnknownToken          = 1002,
  UnexpectedToken       = 1003,
  InvalidTypeAnnotation = 1004,
  InvalidIdentifier     = 1005,
  EarlyEof              = 1006,
  UnboundName           = 1007,
  NameAlreadyBound      = 1008,
  ArgsCountNoMatch      = 1009,
  TypesNoMatch          = 1010,
  UnusedName            = 1011,
  InvalidType           = 1012,
  UnusedValue           = 1013,
  MissingValue          = 1014,
  DeadCode              = 1015,
  CallNotCallable       = 1016,
  BadFnDecl             = 1017,
  DuplicatedParamName   = 1018,
};

class DiagnosticEngine
{
public:
  std::vector<std::string> Errors;

  DiagnosticEngine(std::filesystem::path &filePath, std::string &fileContent)
      : Errors(), FilePath(filePath), FileContent(fileContent) {};

  void PushError(ErrorCode, std::string, Location);
  [[noreturn]] void ErrorAndExit(ErrorCode, std::string, Location);
  std::string HandleError(ErrorCode, std::string, Location);

private:
  const std::filesystem::path &FilePath;
  const std::string &FileContent;
};
