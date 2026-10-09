#include "symbol_table.h"
#include "string_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void symbol_table_init(SymbolTable *table) {
  table->head = NULL;
  table->scope = 0;
  table->scopeName = "global";
}

void symbol_table_enter_scope(SymbolTable *table, const char *name) {
  table->scope++;
  table->scopeName = name;
}

void symbol_table_exit_scope(SymbolTable *table) {
  for (Symbol *symbol = table->head; symbol != NULL; symbol = symbol->next) {
    if (symbol->scope == table->scope) {
      symbol->visible = 0;
    }
  }
  table->scope--;
  table->scopeName = "global";
}

Symbol *symbol_table_lookup(const SymbolTable *table, const char *name) {
  Symbol *found = NULL;
  for (Symbol *symbol = table->head; symbol != NULL; symbol = symbol->next) {
    if (symbol->visible && strcmp(symbol->name, name) == 0 &&
        (found == NULL || symbol->scope >= found->scope)) {
      found = symbol;
    }
  }
  return found;
}

Symbol *symbol_table_lookup_current(const SymbolTable *table, const char *name) {
  Symbol *symbol = symbol_table_lookup(table, name);
  if (symbol != NULL && symbol->scope == table->scope) {
    return symbol;
  }
  return NULL;
}

Symbol *symbol_table_insert(SymbolTable *table, const char *name, DataType type,
                            SymbolKind kind, int line) {
  Symbol *symbol = malloc(sizeof(*symbol));
  if (symbol == NULL) {
    return NULL;
  }
  symbol->name = string_copy(name);
  if (symbol->name == NULL) {
    free(symbol);
    return NULL;
  }
  symbol->type = type;
  symbol->kind = kind;
  symbol->scopeName = table->scopeName;
  symbol->scope = table->scope;
  symbol->visible = 1;
  symbol->line = line;
  symbol->hasValue = 0;
  symbol->value.intValue = 0;
  symbol->function = NULL;
  symbol->next = NULL;

  if (table->head == NULL) {
    table->head = symbol;
  } else {
    Symbol *last = table->head;
    while (last->next != NULL) {
      last = last->next;
    }
    last->next = symbol;
  }
  return symbol;
}

static void value_text(const Symbol *symbol, char *buffer, size_t size) {
  if (!symbol->hasValue) {
    snprintf(buffer, size, "-");
    return;
  }
  switch (symbol->type) {
  case TYPE_RECURSO:
  case TYPE_EPOCA:
  case TYPE_MORAL: snprintf(buffer, size, "%ld", symbol->value.intValue); break;
  case TYPE_HEROE: snprintf(buffer, size, "\"%s\"", symbol->value.strValue); break;
  case TYPE_ALIADO: snprintf(buffer, size, "%s", symbol->value.boolValue ? "paz" : "guerra"); break;
  case TYPE_NADA:
  case TYPE_UNKNOWN: snprintf(buffer, size, "?"); break;
  }
}

static const char *kind_name(SymbolKind kind) {
  switch (kind) {
  case SYMBOL_UNIT: return "unidad";
  case SYMBOL_PARAMETER: return "parametro";
  case SYMBOL_FUNCTION: return "estrategia";
  }
  return "?";
}

void symbol_table_print(const SymbolTable *table) {
  const char *line = "+-----+--------------+------------+----------+--------------+------------+------------------+\n";
  int number = 0;
  char value[64];
  printf("%s", line);
  printf("| %-3s | %-12s | %-10s | %-8s | %-12s | %-10s | %-16s |\n", "#", "Nombre", "Clase",
         "Tipo", "Ambito", "Linea", "Valor");
  printf("%s", line);
  for (const Symbol *symbol = table->head; symbol != NULL; symbol = symbol->next) {
    value_text(symbol, value, sizeof(value));
    printf("| %-3d | %-12s | %-10s | %-8s | %-12s | linea %-4d | %-16s |\n", ++number,
           symbol->name, kind_name(symbol->kind), data_type_name(symbol->type),
           symbol->scopeName, symbol->line, value);
  }
  if (number == 0) {
    printf("| %-89s |\n", "(no se recluto ninguna unidad)");
  }
  printf("%s", line);
}

void symbol_table_destroy(SymbolTable *table) {
  Symbol *symbol = table->head;
  while (symbol != NULL) {
    Symbol *next = symbol->next;
    if (symbol->type == TYPE_HEROE && symbol->hasValue) {
      free(symbol->value.strValue);
    }
    free(symbol->name);
    free(symbol);
    symbol = next;
  }
  table->head = NULL;
}
