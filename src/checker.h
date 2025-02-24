#pragma once

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "ast.h"
#include "diagnostic.h"
#include "loader.h"

class binding
{
public:
  bool is_used;

  binding() = default;
};

class context
{
public:
  std::unordered_map<std::string, std::shared_ptr<binding>> store;

  context() : store() {};

  void save(std::string key, std::shared_ptr<binding> bind)
  {
    store[key] = bind;
  }

  std::optional<std::shared_ptr<binding>> get(std::string key)
  {
    if (store.find(key) == store.end())
      return std::nullopt;
    return store[key];
  }
};

class checker
{
public:
  checker(ast tree_, std::shared_ptr<source> s) : ctx(), tree(tree_), src(s) {};

  std::vector<diagnostic> check();

private:
  context ctx;
  ast tree;
  std::shared_ptr<source> src;

  std::vector<diagnostic> diagnostics;

  std::vector<diagnostic> check_stmt(std::shared_ptr<stmt>);
  std::vector<diagnostic> check_expr(std::shared_ptr<expr>);
  std::vector<diagnostic> check_expr_ident(std::shared_ptr<expr_ident>);
  std::vector<diagnostic> check_expr_call(std::shared_ptr<expr_call>);
  std::vector<diagnostic> check_expr_string(std::shared_ptr<expr_string>);
};
