#include "AstVisitor.h"
#include "Ast.h"

#include <format>

void AstInspector::Write(std::string string)
{
    for (unsigned int i = 0; i < TabSize; ++i)
    {
        Out << " ";
    }
    Out << string << std::endl;
}

void AstInspector::Tab() { TabSize += TabRate; }
void AstInspector::UnTab() { TabSize -= TabRate; }

std::any AstInspector::visit(BlockStatement *block)
{
    Write("Block Statement:");
    Tab();
    for (auto &x : block->Statements)
    {
        x->accept(this);
    }
    UnTab();
    return 0;
}

std::any AstInspector::visit(FunctionStatement *function)
{
    Write("Function Statement:");
    Tab();
    Write(std::format("Name: \"{}\"", function->GetName()));
    Write("Body:");
    Tab();
    function->Body.accept(this);
    UnTab();
    UnTab();
    return 0;
}

std::any AstInspector::visit(CallExpression *call)
{
    Write("Call Expression:");
    Tab();

    Write("Callee:");
    Tab();
    call->Callee->accept(this);
    UnTab();

    Write("Arguments: [");
    Tab();
    for (auto &x : call->Args)
    {
        x->accept(this);
    }
    UnTab();
    Write("]");

    UnTab();
    return 0;
}

std::any AstInspector::visit(IdentifierExpression *ident)
{
    Write(std::format("Identifier: {}", ident->GetValue()));
    return 0;
}

std::any AstInspector::visit(StringExpression *string)
{
    Write(std::format("String: \"{}\"", string->GetValue()));
    return 0;
}