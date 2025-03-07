#pragma once

#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace type
{
enum class Base
{
  BYTE,
  VOID,
  /* 'string' is interchangeble with 'format string' */
  STRING,
  FUNCTION,

  // integers
  I8,
  I16,
  I32,
  I64,
  U8,
  U16,
  U32,
  U64,
  // floats
  F8,
  F16,
  F32,
  F64,

  // internal
  ANY,
  /* 'format string' is interchangeble with 'string' */
  OBJECT,
};

class Type
{
public:
  Base m_Base;

  Type(Base base) : m_Base(base) {}
  virtual ~Type() = default;

  virtual std::string Inspect() const;
  virtual bool IsCompatibleWith(std::shared_ptr<Type>) const;

  static std::shared_ptr<Type> make_byte();
  static std::shared_ptr<Type> make_void();
  static std::shared_ptr<Type> make_string();
  static std::shared_ptr<Type> make_i8();
  static std::shared_ptr<Type> make_i16();
  static std::shared_ptr<Type> make_i32();
  static std::shared_ptr<Type> make_i64();
  static std::shared_ptr<Type> make_u8();
  static std::shared_ptr<Type> make_u16();
  static std::shared_ptr<Type> make_u32();
  static std::shared_ptr<Type> make_u64();
  static std::shared_ptr<Type> make_f8();
  static std::shared_ptr<Type> make_f16();
  static std::shared_ptr<Type> make_f32();
  static std::shared_ptr<Type> make_f64();
};

class Function : public Type
{
public:
  size_t m_ReqArgsCount;
  std::vector<std::shared_ptr<Type>> m_Arguments;
  std::shared_ptr<Type> m_ReturnType;
  bool m_IsVariadicArguments;

  std::string Inspect() const override;
  bool IsCompatibleWith(std::shared_ptr<Type>) const override;

  Function(size_t reqArgsCount, std::vector<std::shared_ptr<Type>> arguments, std::shared_ptr<Type> returnType, bool isVariadic = false) : Type(Base::FUNCTION), m_ReqArgsCount(reqArgsCount), m_Arguments(std::move(arguments)), m_ReturnType(returnType), m_IsVariadicArguments(isVariadic) {}
};

class Object : public Type
{
public:
  std::map<std::string, std::shared_ptr<Type>> m_Entries;

  std::string Inspect() const override;
  bool IsCompatibleWith(std::shared_ptr<Type>) const override;

  Object() : Type(Base::OBJECT), m_Entries() {};
};

std::optional<std::shared_ptr<Type>> NarrowTypes(std::shared_ptr<Type>, std::shared_ptr<Type>);

} // namespace type
