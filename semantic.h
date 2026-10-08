#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"
#include "symbol_table.h"

int semantic_analysis(ASTNode *program, SymbolTable *table);

#endif
