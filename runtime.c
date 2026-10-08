#include "runtime.h"
#include "string_utils.h"

#include <stdio.h>
#include <stdlib.h>

static void set_default(Symbol *symbol) {
  switch (symbol->type) {
  case TYPE_RECURSO: symbol->value.intValue = 0; break;
  case TYPE_EPOCA: symbol->value.intValue = 1; break;
  case TYPE_MORAL: symbol->value.intValue = 0; break;
  case TYPE_HEROE: symbol->value.strValue = string_copy(""); break;
  case TYPE_ALIADO: symbol->value.boolValue = 0; break;
  case TYPE_UNKNOWN: break;
  }
  symbol->hasValue = 1;
}

static int store(Symbol *target, const ASTNode *value, const SymbolTable *table) {
  long int_value = 0;
  const char *text = "";
  int bool_value = 0;

  if (value->type == IDENTIFIER_REF) {
    const Symbol *source = symbol_table_lookup(table, value->data.identifier);
    switch (source->type) {
    case TYPE_RECURSO:
    case TYPE_EPOCA:
    case TYPE_MORAL: int_value = source->value.intValue; break;
    case TYPE_HEROE: text = source->value.strValue; break;
    case TYPE_ALIADO: bool_value = source->value.boolValue; break;
    case TYPE_UNKNOWN: break;
    }
  } else {
    switch (value->type) {
    case INT_LITERAL: int_value = value->data.intValue; break;
    case STR_LITERAL: text = value->data.strValue; break;
    case BOOL_LITERAL: bool_value = value->data.boolValue; break;
    default: break;
    }
  }

  switch (target->type) {
  case TYPE_EPOCA:
    if (int_value < 1 || int_value > 14) {
      fprintf(stderr, "DERROTA en la linea %d: solo existen 14 epocas ('%s' quedaria en %ld)\n",
              value->line, target->name, int_value);
      return 0;
    }
    target->value.intValue = int_value;
    break;
  case TYPE_MORAL:
    if (int_value < 0 || int_value > 5) {
      fprintf(stderr, "DERROTA en la linea %d: la moral solo va de 0 a 5 ('%s' quedaria en %ld)\n",
              value->line, target->name, int_value);
      return 0;
    }
    target->value.intValue = int_value;
    break;
  case TYPE_RECURSO: target->value.intValue = int_value; break;
  case TYPE_HEROE: {
    char *copy = string_copy(text);
    free(target->value.strValue);
    target->value.strValue = copy;
    break;
  }
  case TYPE_ALIADO: target->value.boolValue = bool_value; break;
  case TYPE_UNKNOWN: break;
  }
  return 1;
}

int execute_program(const ASTNode *program, SymbolTable *table) {
  for (const ASTNode *node = program; node != NULL; node = node->next) {
    if (node->type == VAR_DECL) {
      Symbol *symbol = symbol_table_lookup(table, node->data.varDeclaration.identifier);
      set_default(symbol);
      if (node->data.varDeclaration.init != NULL &&
          !store(symbol, node->data.varDeclaration.init, table)) {
        return 0;
      }
    } else if (node->type == ASSIGNMENT) {
      Symbol *symbol = symbol_table_lookup(table, node->data.assignment.identifier);
      if (!store(symbol, node->data.assignment.value, table)) {
        return 0;
      }
    }
  }
  return 1;
}
