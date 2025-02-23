#include <cstddef>
#include <format>
#include <memory>
#include <sstream>
#include <string>

#include "ast.h"
#include "token.h"

class ast_inspector
{
public:
  ast_inspector(ast *t) : tree(t), tabrate(4), tabsize(0), oss() {}

  std::string inspect();

private:
  ast *tree;

  size_t tabrate;
  size_t tabsize;
  std::ostringstream oss;

  void tab();
  void untab();
  void write(std::string);
  void writeln(std::string);

  void inspect_stmt(std::shared_ptr<statement>);
  void inspect_expr(std::shared_ptr<expression>);
  void inspect_expr_call(std::shared_ptr<expression_call>);
  void inspect_expr_string(std::shared_ptr<expression_string>);
  void inspect_expr_identifier(std::shared_ptr<expression_identifier>);
};

void ast_inspector::tab()
{
  tabsize += tabrate;
}

void ast_inspector::untab()
{
  tabsize -= tabrate;
}

void ast_inspector::write(std::string str)
{
  oss << std::string(tabsize, ' ') << str;
}

void ast_inspector::writeln(std::string str)
{
  write(str);
  oss << '\n';
}

std::string ast_inspector::inspect()
{
  oss << "Abstract Syntax Tree\n\n";

  for (std::shared_ptr<statement> stmt : tree->program)
  {
    inspect_stmt(stmt);
  }

  return oss.str();
}

void ast_inspector::inspect_stmt(std::shared_ptr<statement> stmt)
{
  switch (stmt.get()->type)
  {
  case statement_type::expr:
    inspect_expr(std::static_pointer_cast<expression>(stmt));
  }
}

void ast_inspector::inspect_expr(std::shared_ptr<expression> expr)
{
  switch (expr.get()->type)
  {
  case expression_type::call:
    return inspect_expr_call(std::static_pointer_cast<expression_call>(expr));
  case expression_type::string:
    return inspect_expr_string(std::static_pointer_cast<expression_string>(expr));
  case expression_type::identifier:
    return inspect_expr_identifier(std::static_pointer_cast<expression_identifier>(expr));
  }
}

void ast_inspector::inspect_expr_call(std::shared_ptr<expression_call> expr_call)
{
  position p = expr_call.get()->pos;
  writeln(std::format("call expression: {{{}:{}:{}:{}}}", p.line, p.col, p.start, p.end));
  tab();

  writeln("callee:");
  tab();
  inspect_expr(expr_call.get()->callee);
  untab();

  writeln("arguments: [");
  tab();
  for (auto arg : expr_call.get()->args)
  {
    inspect_expr(arg);
  }
  untab();
  writeln("]");

  untab();
}

void ast_inspector::inspect_expr_string(std::shared_ptr<expression_string> expr_string)
{
  writeln(std::format("string literal: {}", expr_string.get()->value));
}

void ast_inspector::inspect_expr_identifier(std::shared_ptr<expression_identifier> expr_identifier)
{
  writeln(std::format("identifer expression: {}", expr_identifier.get()->value));
}

std::string ast::inspect()
{
  ast_inspector ai(this);
  return ai.inspect();
}
