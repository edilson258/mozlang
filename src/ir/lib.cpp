#include "lib.h"

namespace lib
{
uint32_t Pool::Save(Object object)
{
  uint32_t index = static_cast<uint32_t>(m_Objects.size());
  m_Objects.push_back(object);
  return index;
}

std::optional<Object> Pool::Get(uint32_t index) const
{
  if (index >= m_Objects.size())
  {
    return std::nullopt;
  }
  return m_Objects.at(index);
}

} // namespace lib
