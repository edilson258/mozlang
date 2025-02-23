#pragma once

#include <variant>

template <typename T, typename E>
class result
{
public:
  result(T value) : inner(std::in_place_type<T>, value) {};
  result(E error) : inner(std::in_place_type<E>, error) {};

  T unwrap()
  {
    return std::get<T>(inner);
  }

  E unwrap_err()
  {
    return std::get<E>(inner);
  }

  void set_val(T val)
  {
    inner = val;
  }

  bool is_ok()
  {
    return inner.index() == 0;
  }

  bool is_err()
  {
    return !is_ok();
  }

private:
  std::variant<T, E> inner;
};
