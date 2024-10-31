#include "Ast.h"

std::string AST::ToStrng()
{
    AstInspector inspector;
    for (auto &x : Nodes)
    {
        x->accept(&inspector);
    }

    return inspector.GetValue();
}

std::any FunctionStatement::accept(AstVisitor *visitor) { return visitor->visit(this); }
std::string FunctionStatement::GetName() const { return std::get<std::string>(Identifier.Data); }

std::any BlockStatement::accept(AstVisitor *visitor) { return visitor->visit(this); }

std::any IdentifierExpression::accept(AstVisitor *visitor) { return visitor->visit(this); }
std::string IdentifierExpression::GetValue() const { return std::get<std::string>(Identifier.Data); }

std::any StringExpression::accept(AstVisitor *visitor) { return visitor->visit(this); }
std::string StringExpression::GetValue() const { return std::get<std::string>(String.Data); }

std::any CallExpression::accept(AstVisitor *visitor) { return visitor->visit(this); }