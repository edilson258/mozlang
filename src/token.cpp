#include <sstream>

#include "token.h"

std::string token::inspect()
{
  std::ostringstream oss;
  oss << lexeme << " ";
  oss << pos.line << ":" << pos.col << ":" << pos.start << ":" << pos.end;
  return oss.str();
}
