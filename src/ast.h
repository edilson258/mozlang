#pragma once

#include <memory>
#include <string>
#include <vector>

#include "token.h"

enum class precedence
{
  lowest = 0,
  call = 10,
};

enum class stmt_t
{
  expr = 1,
};

enum class expr_t
{
  call = 1,
  ident,
  string,
};

class stmt
{
public:
  position pos;
  stmt_t type;

protected:
  stmt(position p, stmt_t t) : pos(p), type(t) {};
};

class expr : public stmt
{
public:
  expr_t type;

protected:
  expr(position p, expr_t t) : stmt(p, stmt_t::expr), type(t) {};
};

class expr_call : public expr
{
public:
  std::shared_ptr<expr> callee;
  std::vector<std::shared_ptr<expr>> args;

  expr_call(position p, std::shared_ptr<expr> c, std::vector<std::shared_ptr<expr>> as) : expr(p, expr_t::call), callee(c), args(as) {};
};

class expr_ident : public expr
{
public:
  std::string value;

  expr_ident(position p, std::string v) : expr(p, expr_t::ident), value(v) {};
};

class expr_string : public expr
{
public:
  std::string value;

  expr_string(position p, std::string v) : expr(p, expr_t::string), value(v) {};
};

class ast
{
public:
  ast() : program() {};

  std::vector<std::shared_ptr<stmt>> program;

  std::string inspect();
};
