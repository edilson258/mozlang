#include "keywords.h"

std::optional<TokenType> Keyword::match(std::string identifier)
{
  if (KEYWORDS.contains(identifier))
  {
    return KEYWORDS.at(identifier);
  }
  return std::nullopt;
}
