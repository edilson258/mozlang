#pragma once

#include <cstddef>
#include <functional>
#include <string>

#include "diagnostic.h"
#include "module.h"
#include "result.h"
#include "token.h"

class Lexer
{
public:
  Lexer(ModuleID moduleID, ModuleManager &moduleManager) : m_ModuleID(moduleID), m_ModManager(moduleManager), m_ModuleContent(m_ModManager.m_Modules[moduleID]->m_Content), m_Line(1), m_Column(1), m_Cursor(0) {};

  Result<Token, Diagnostic> Next();

private:
  ModuleID m_ModuleID;
  ModuleManager &m_ModManager;
  std::string &m_ModuleContent;

  size_t m_Line;
  size_t m_Column;
  size_t m_Cursor;

  bool IsEof();
  char PeekOne();
  void Advance();
  void Advance(size_t);
  size_t AdvanceWhile(std::function<bool(char)>);
  bool StartsWith(std::string);

  Result<Token, Diagnostic> MakeTokenString();
  Result<Token, Diagnostic> MakeTokenSimple(TokenType);
  Result<Token, Diagnostic> MakeIfNextOr(std::string, TokenType, TokenType);
};
