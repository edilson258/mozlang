#pragma once

#include <variant>

template <typename T, typename E>
class Result
{
public:
  Result(T value) : inner(std::in_place_type<T>, value) {};
  Result(E error) : inner(std::in_place_type<E>, error) {};

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
    inner.template emplace<T>(val);
  }

  bool is_ok()
  {
    if (std::holds_alternative<T>(inner))
    {
      return true;
    }
    return false;
  }

  bool is_err()
  {
    return !is_ok();
  }

private:
  std::variant<T, E> inner;
};
