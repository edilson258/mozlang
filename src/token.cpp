#include <format>

#include "token.h"

std::string Token::Inspect()
{
  return std::format("{} {}:{}:{}:{}", m_Lexeme, m_Position.m_Line, m_Position.m_Column, m_Position.m_Start, m_Position.m_End);
}
