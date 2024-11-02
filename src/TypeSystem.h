#pragma once

enum class TypeOfType
{
  Int,
};

class Type
{
public:
  TypeOfType Typ;

  Type(TypeOfType type) : Typ(type) {};
};
