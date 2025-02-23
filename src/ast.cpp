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

  void inspect_stmt(std::shared_ptr<stmt>);
  void inspect_expr(std::shared_ptr<expr>);
  void inspect_expr_call(std::shared_ptr<expr_call>);
  void inspect_expr_string(std::shared_ptr<expr_string>);
  void inspect_expr_identifier(std::shared_ptr<expr_ident>);
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

  for (std::shared_ptr<stmt> stmt : tree->program)
  {
    inspect_stmt(stmt);
  }

  return oss.str();
}

void ast_inspector::inspect_stmt(std::shared_ptr<stmt> stmt)
{
  switch (stmt.get()->type)
  {
  case stmt_t::expr:
    inspect_expr(std::static_pointer_cast<expr>(stmt));
  }
}

void ast_inspector::inspect_expr(std::shared_ptr<expr> expr)
{
  switch (expr.get()->type)
  {
  case expr_t::call:
    return inspect_expr_call(std::static_pointer_cast<expr_call>(expr));
  case expr_t::string:
    return inspect_expr_string(std::static_pointer_cast<expr_string>(expr));
  case expr_t::ident:
    return inspect_expr_identifier(std::static_pointer_cast<expr_ident>(expr));
  }
}

void ast_inspector::inspect_expr_call(std::shared_ptr<expr_call> expr_call)
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

void ast_inspector::inspect_expr_string(std::shared_ptr<expr_string> expr_string)
{
  writeln(std::format("string literal: {}", expr_string.get()->value));
}

void ast_inspector::inspect_expr_identifier(std::shared_ptr<expr_ident> expr_identifier)
{
  writeln(std::format("identifer expression: {}", expr_identifier.get()->value));
}

std::string ast::inspect()
{
  ast_inspector ai(this);
  return ai.inspect();
}
