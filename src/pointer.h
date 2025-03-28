#pragma once

#include <memory>

template <typename T>
using Ptr = std::shared_ptr<T>;

template <typename T>
inline Ptr<T> MakePtr(T x)
{
  return std::make_shared<T>(x);
}

template <typename To, typename From>
inline std::shared_ptr<To> CastPtr(std::shared_ptr<From> x)
{
  return std::static_pointer_cast<To>(x);
}
