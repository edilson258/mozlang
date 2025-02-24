#pragma once

#include <cstddef>
#include <memory>
#include <string>

#include "error.h"
#include "loader.h"
#include "token.h"

enum class diagnostic_severity
{
  info = 1,
  warn = 2,
  error = 3,
};

class diagnostic
{
public:
  errn ern;
  position pos;
  std::shared_ptr<source> src;
  diagnostic_severity severity;
  std::string message;

  diagnostic(errn en, position p, std::shared_ptr<source> s, diagnostic_severity sev, std::string msg) : ern(en), pos(p), src(s), severity(sev), message(msg) {};
};

class diagnostic_engine
{
public:
  diagnostic_engine() {}

  void report(diagnostic);

private:
  std::string match_sevevirty_string(diagnostic_severity severity);
  std::string match_severity_color(diagnostic_severity severity);
  std::string paint(std::string text, std::string colr);
  std::string highlight(std::string code, size_t start, size_t end, std::string colr);
};
