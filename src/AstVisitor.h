#pragma once

#include <any>
#include <sstream>
#include <string>

class BlockStatement;
class FunctionStatement;
class CallExpression;
class IdentifierExpression;
class StringExpression;

class AstVisitor
{
public:
  virtual std::any visit(BlockStatement *)       = 0;
  virtual std::any visit(FunctionStatement *)    = 0;
  virtual std::any visit(CallExpression *)       = 0;
  virtual std::any visit(IdentifierExpression *) = 0;
  virtual std::any visit(StringExpression *)     = 0;

  virtual ~AstVisitor() = default;
};

class AstInspector : public AstVisitor
{
public:
  std::any visit(BlockStatement *) override;
  std::any visit(FunctionStatement *) override;
  std::any visit(CallExpression *) override;
  std::any visit(IdentifierExpression *) override;
  std::any visit(StringExpression *) override;

  AstInspector() : TabSize(0), TabRate(4) {};

  std::string GetValue() const { return Out.str(); }

private:
  unsigned long TabSize;
  unsigned long TabRate;

  std::ostringstream Out;

  void Write(std::string);

  void Tab();
  void UnTab();
};