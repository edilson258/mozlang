#pragma once

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
  virtual void *visit(BlockStatement *)       = 0;
  virtual void *visit(FunctionStatement *)    = 0;
  virtual void *visit(CallExpression *)       = 0;
  virtual void *visit(IdentifierExpression *) = 0;
  virtual void *visit(StringExpression *)     = 0;

  virtual ~AstVisitor() = default;
};

class AstInspector : public AstVisitor
{
public:
  void *visit(BlockStatement *) override;
  void *visit(FunctionStatement *) override;
  void *visit(CallExpression *) override;
  void *visit(IdentifierExpression *) override;
  void *visit(StringExpression *) override;

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
