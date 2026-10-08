#ifndef RUNTIME_H
#define RUNTIME_H

#include "ast.h"
#include "symbol_table.h"

int execute_program(const ASTNode *program, SymbolTable *table);

#endif
