#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "ast.h"

typedef enum {
  SYMBOL_UNIT,
  SYMBOL_PARAMETER,
  SYMBOL_FUNCTION
} SymbolKind;

typedef union {
  long intValue;
  double floatValue;
  char *strValue;
  int boolValue;
} SymbolValue;

typedef struct Symbol {
  char *name;
  DataType type;
  SymbolKind kind;
  const char *scopeName;
  unsigned int scope;
  int visible;
  int line;
  int hasValue;
  SymbolValue value;
  const ASTNode *function;
  struct Symbol *next;
} Symbol;

typedef struct {
  Symbol *head;
  unsigned int scope;
  const char *scopeName;
} SymbolTable;

void symbol_table_init(SymbolTable *table);
void symbol_table_enter_scope(SymbolTable *table, const char *name);
void symbol_table_exit_scope(SymbolTable *table);
/* Returns the new symbol, or NULL on allocation failure. */
Symbol *symbol_table_insert(SymbolTable *table, const char *name, DataType type,
                            SymbolKind kind, int line);
Symbol *symbol_table_lookup(const SymbolTable *table, const char *name);
Symbol *symbol_table_lookup_current(const SymbolTable *table, const char *name);
void symbol_table_print(const SymbolTable *table);
void symbol_table_destroy(SymbolTable *table);

#endif
