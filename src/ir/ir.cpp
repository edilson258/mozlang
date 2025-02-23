#include <string>

#include "ir/ir.h"
#include "result.h"

result<std::string, error> ir_generator::emit()
{
  return result<std::string, error>(oss.str());
}
