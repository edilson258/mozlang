#pragma once

#include <sstream>
#include <string>

#include "ast.h"
#include "error.h"
#include "result.h"

enum class op_code
{
  load = 1,
  store,
  invoke,
};

class ir_generator
{
public:
  ir_generator(ast &t) : tree(t), oss() {};

  result<std::string, error> emit();

private:
  ast &tree;
  std::ostringstream oss;
};
