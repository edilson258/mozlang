#pragma once

#include <sstream>
#include <string>

class BlockStatement;
class FunctionStatement;
class ReturnStatement;
class CallExpression;
class IdentifierExpression;
class StringExpression;
class IntegerExpression;
class BooleanExpression;

class AstVisitor
{
public:
  virtual void *visit(BlockStatement *)       = 0;
  virtual void *visit(FunctionStatement *)    = 0;
  virtual void *visit(ReturnStatement *)      = 0;
  virtual void *visit(CallExpression *)       = 0;
  virtual void *visit(IdentifierExpression *) = 0;
  virtual void *visit(StringExpression *)     = 0;
  virtual void *visit(IntegerExpression *)    = 0;
  virtual void *visit(BooleanExpression *)    = 0;

  virtual ~AstVisitor() = default;
};

class AstInspector : public AstVisitor
{
public:
  void *visit(BlockStatement *) override;
  void *visit(FunctionStatement *) override;
  void *visit(ReturnStatement *) override;
  void *visit(CallExpression *) override;
  void *visit(IdentifierExpression *) override;
  void *visit(StringExpression *) override;
  void *visit(IntegerExpression *) override;
  void *visit(BooleanExpression *) override;

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
