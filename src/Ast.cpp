#include "Ast.h"
#include <string>

std::string AST::ToStrng()
{
  AstInspector inspector;
  for (auto &node : Nodes)
  {
    node->accept(&inspector);
  }

  return inspector.GetValue();
}

void *FunctionStatement::accept(AstVisitor *visitor) { return visitor->visit(this); }
void *ReturnStatement::accept(AstVisitor *visitor) { return visitor->visit(this); }

void *BlockStatement::accept(AstVisitor *visitor) { return visitor->visit(this); }

void *IdentifierExpression::accept(AstVisitor *visitor) { return visitor->visit(this); }
std::string IdentifierExpression::GetValue() const { return Identifier.Data.value(); }

void *StringExpression::accept(AstVisitor *visitor) { return visitor->visit(this); }
std::string StringExpression::GetValue() const { return String.Data.value(); }

void *IntegerExpression::accept(AstVisitor *visitor) { return visitor->visit(this); }
std::string IntegerExpression::GetRawValue() const { return Integer.Data.value(); }
long long IntegerExpression::GetValue() const { return std::stoll(Integer.Data.value()); }

void *CallExpression::accept(AstVisitor *visitor) { return visitor->visit(this); }
