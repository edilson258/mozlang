#pragma once

#include <stdexcept>
#include <variant>

template <typename T, typename E>
class Result
{
public:
  using m_ValueType = T;
  using m_ErrorType = E;

private:
  std::variant<T, E> value;

public:
  Result(T val) : value(std::move(val)) {}
  Result(E err) : value(std::move(err)) {}

  bool is_ok() const { return std::holds_alternative<T>(value); }
  bool is_err() const { return std::holds_alternative<E>(value); }

  T &unwrap()
  {
    if (!is_ok())
      throw std::runtime_error("Called unwrap on an error!");
    return std::get<T>(value);
  }

  E &unwrap_err()
  {
    if (!is_err())
      throw std::runtime_error("Called unwrap_err on a valid result!");
    return std::get<E>(value);
  }

  void set_val(T val)
  {
    value.template emplace<T>(val);
  }
};

// int main() {
//     Result<int, std::string> success(42);
//     Result<int, std::string> failure(std::string("Something went wrong"));

//     if (success.is_ok()) {
//         std::cout << "Success: " << success.unwrap() << '\n';
//     }

//     if (failure.is_err()) {
//         std::cout << "Error: " << failure.unwrap_err() << '\n';
//     }

//     return 0;
// }

// template <typename T, typename E>
// class Result
// {
// public:
//   Result(T value) : inner(std::in_place_type<T>, value) {};
//   Result(E error) : inner(std::in_place_type<E>, error) {};

//   T unwrap()
//   {
//     return std::get<T>(inner);
//   }

//   E unwrap_err()
//   {
//     return std::get<E>(inner);
//   }

//   void set_val(T val)
//   {
//     inner.template emplace<T>(val);
//   }

//   bool is_ok()
//   {
//     if (std::holds_alternative<T>(inner))
//     {
//       return true;
//     }
//     return false;
//   }

//   bool is_err()
//   {
//     return !is_ok();
//   }

// private:
//   std::variant<T, E> inner;
// };
