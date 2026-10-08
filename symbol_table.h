#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "ast.h"

typedef union {
  long intValue;
  double floatValue;
  char *strValue;
  int boolValue;
} SymbolValue;

typedef struct Symbol {
  char *name;
  DataType type;
  int line;
  int hasValue;
  SymbolValue value;
  struct Symbol *next;
} Symbol;

typedef struct {
  Symbol *head;
} SymbolTable;

void symbol_table_init(SymbolTable *table);
/* Returns 1 when inserted, 0 for a duplicate, and -1 on allocation failure. */
int symbol_table_insert(SymbolTable *table, const char *name, DataType type, int line);
Symbol *symbol_table_lookup(const SymbolTable *table, const char *name);
void symbol_table_print(const SymbolTable *table);
void symbol_table_destroy(SymbolTable *table);

#endif
