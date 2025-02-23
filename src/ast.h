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

enum class statement_type
{
  expr = 1,
};

enum class expression_type
{
  call = 1,
  identifier,
  string,
};

class statement
{
public:
  position pos;
  statement_type type;

protected:
  statement(position p, statement_type t) : pos(p), type(t) {};
};

class expression : public statement
{
public:
  expression_type type;

protected:
  expression(position p, expression_type t) : statement(p, statement_type::expr), type(t) {};
};

class expression_call : public expression
{
public:
  std::shared_ptr<expression> callee;
  std::vector<std::shared_ptr<expression>> args;

  expression_call(position p, std::shared_ptr<expression> c, std::vector<std::shared_ptr<expression>> as) : expression(p, expression_type::call), callee(c), args(as) {};
};

class expression_identifier : public expression
{
public:
  std::string value;

  expression_identifier(position p, std::string v) : expression(p, expression_type::identifier), value(v) {};
};

class expression_string : public expression
{
public:
  std::string value;

  expression_string(position p, std::string v) : expression(p, expression_type::string), value(v) {};
};

class ast
{
public:
  ast() : program() {};

  std::vector<std::shared_ptr<statement>> program;

  std::string inspect();
};
