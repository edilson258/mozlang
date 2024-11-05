#include "AstVisitor.h"
#include "Ast.h"

#include <format>
#include <string>

void AstInspector::Write(std::string string)
{
  Out << std::string(TabSize, ' ');
  Out << string << std::endl;
}

void AstInspector::Tab() { TabSize += TabRate; }
void AstInspector::UnTab() { TabSize -= TabRate; }

void *AstInspector::visit(BlockStatement *block)
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

void *AstInspector::visit(FunctionStatement *function)
{
  Write("Function Statement:");
  Tab();
  Write(std::format("Name: \"{}\"", function->Identifier->GetValue()));
  Write("Parameters: [");
  Tab();
  for (FunctionParam &p : function->Params)
  {
    Write(p.Identifier->GetValue());
  }
  UnTab();
  Write("]");
  Write("Body:");
  Tab();
  function->Body.accept(this);
  UnTab();
  UnTab();
  return 0;
}

void *AstInspector::visit(ReturnStatement *ret)
{
  Write("Return Statement:");
  Tab();
  Write("Value:");
  Tab();
  if (ret->Value.has_value())
  {
    ret->Value.value()->accept(this);
  }
  else
  {
    Write("<NO VALUE>");
  }
  UnTab();
  UnTab();
  return 0;
}

void *AstInspector::visit(CallExpression *call)
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

void *AstInspector::visit(IdentifierExpression *ident)
{
  Write(std::format("Identifier: {}", ident->GetValue()));
  return 0;
}

void *AstInspector::visit(StringExpression *string)
{
  Write(std::format("String: \"{}\"", string->GetValue()));
  return 0;
}

void *AstInspector::visit(IntegerExpression *integer)
{
  Write(std::format("Integer: {}", integer->GetValue()));
  return 0;
}
