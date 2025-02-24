#include <format>
#include <memory>
#include <vector>

#include "ast.h"
#include "checker.h"
#include "error.h"

std::vector<diagnostic> checker::check()
{
  for (auto stmt : tree.program)
  {
    auto xs = check_stmt(stmt);
    diagnostics.insert(diagnostics.end(), xs.begin(), xs.end());
  }
  return diagnostics;
}

std::vector<diagnostic> checker::check_stmt(std::shared_ptr<stmt> stm)
{
  std::vector<diagnostic> diags;
  switch (stm.get()->type)
  {
  case stmt_t::expr:
  {
    auto xs = check_expr(std::static_pointer_cast<expr>(stm));
    diags.insert(diags.end(), xs.begin(), xs.end());
    break;
  }
  }
  return diags;
}

std::vector<diagnostic> checker::check_expr(std::shared_ptr<expr> exp)
{
  std::vector<diagnostic> diags;
  switch (exp.get()->type)
  {
  case expr_t::call:
  {
    auto xs = check_expr_call(std::static_pointer_cast<expr_call>(exp));
    diags.insert(diags.end(), xs.begin(), xs.end());
    break;
  }
  case expr_t::string:
  {
    auto xs = check_expr_string(std::static_pointer_cast<expr_string>(exp));
    diags.insert(diags.end(), xs.begin(), xs.end());
    break;
  }
  case expr_t::ident:
  {
    auto xs = check_expr_ident(std::static_pointer_cast<expr_ident>(exp));
    diags.insert(diags.end(), xs.begin(), xs.end());
    break;
  }
  }
  return diags;
}

std::vector<diagnostic> checker::check_expr_call(std::shared_ptr<expr_call> exp_call)
{
  auto diags = check_expr(exp_call.get()->callee);
  for (auto arg : exp_call.get()->args)
  {
    auto xs = check_expr(arg);
    diags.insert(diags.end(), xs.begin(), xs.end());
  }
  return diags;
}

std::vector<diagnostic> checker::check_expr_ident(std::shared_ptr<expr_ident> exp_ident)
{
  std::vector<diagnostic> diags;
  auto x = ctx.get(exp_ident.get()->value);
  if (!x.has_value())
  {
    diags.push_back(diagnostic(errn::name_error, exp_ident.get()->pos, src, diagnostic_severity::error, std::format("undefined name: '{}'", exp_ident.get()->value)));
  }
  return diags;
}

std::vector<diagnostic> checker::check_expr_string(std::shared_ptr<expr_string>)
{
  return {};
}
