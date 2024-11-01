#include "Ast.h"

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

void *BlockStatement::accept(AstVisitor *visitor) { return visitor->visit(this); }

void *IdentifierExpression::accept(AstVisitor *visitor) { return visitor->visit(this); }
std::string IdentifierExpression::GetValue() const { return std::get<std::string>(Identifier.Data); }

void *StringExpression::accept(AstVisitor *visitor) { return visitor->visit(this); }
std::string StringExpression::GetValue() const { return std::get<std::string>(String.Data); }

void *CallExpression::accept(AstVisitor *visitor) { return visitor->visit(this); }
