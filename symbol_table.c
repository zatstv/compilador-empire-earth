#include "symbol_table.h"
#include "string_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void symbol_table_init(SymbolTable *table) { table->head = NULL; }

Symbol *symbol_table_lookup(const SymbolTable *table, const char *name) {
  for (Symbol *symbol = table->head; symbol != NULL; symbol = symbol->next) {
    if (strcmp(symbol->name, name) == 0) {
      return symbol;
    }
  }
  return NULL;
}

int symbol_table_insert(SymbolTable *table, const char *name, DataType type, int line) {
  if (symbol_table_lookup(table, name) != NULL) {
    return 0;
  }

  Symbol *symbol = malloc(sizeof(*symbol));
  if (symbol == NULL) {
    return -1;
  }
  symbol->name = string_copy(name);
  if (symbol->name == NULL) {
    free(symbol);
    return -1;
  }
  symbol->type = type;
  symbol->line = line;
  symbol->hasValue = 0;
  symbol->value.intValue = 0;
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
  return 1;
}

static void value_text(const Symbol *symbol, char *buffer, size_t size) {
  if (!symbol->hasValue) {
    snprintf(buffer, size, "-");
    return;
  }
  switch (symbol->type) {
  case TYPE_RECURSO:
  case TYPE_EPOCA: snprintf(buffer, size, "%ld", symbol->value.intValue); break;
  case TYPE_MORAL: snprintf(buffer, size, "%.2f", symbol->value.floatValue); break;
  case TYPE_HEROE: snprintf(buffer, size, "\"%s\"", symbol->value.strValue); break;
  case TYPE_ALIADO: snprintf(buffer, size, "%s", symbol->value.boolValue ? "paz" : "guerra"); break;
  case TYPE_UNKNOWN: snprintf(buffer, size, "?"); break;
  }
}

void symbol_table_print(const SymbolTable *table) {
  const char *line = "+-----+--------------+----------+--------------+------------------+\n";
  int number = 0;
  char value[64];
  printf("%s", line);
  printf("| %-3s | %-12s | %-8s | %-12s | %-16s |\n", "#", "Nombre", "Tipo", "Reclutada en",
         "Valor");
  printf("%s", line);
  for (const Symbol *symbol = table->head; symbol != NULL; symbol = symbol->next) {
    value_text(symbol, value, sizeof(value));
    printf("| %-3d | %-12s | %-8s | linea %-6d | %-16s |\n", ++number, symbol->name,
           data_type_name(symbol->type), symbol->line, value);
  }
  if (number == 0) {
    printf("| %-64s |\n", "(no se recluto ninguna unidad)");
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
