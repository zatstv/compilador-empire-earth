#include "semantic.h"

#include <stdarg.h>
#include <stdio.h>

static int errors;

static void defeat(int line, const char *format, ...) {
  va_list args;
  va_start(args, format);
  fprintf(stderr, "DERROTA en la linea %d: ", line);
  vfprintf(stderr, format, args);
  fprintf(stderr, "\n");
  va_end(args);
  errors++;
}

static const char *article(DataType type) {
  return (type == TYPE_EPOCA || type == TYPE_MORAL) ? "una" : "un";
}

static ValueKind kind_of_type(DataType type) {
  switch (type) {
  case TYPE_RECURSO:
  case TYPE_EPOCA:
  case TYPE_MORAL: return VALUE_INT;
  case TYPE_HEROE: return VALUE_TEXT;
  case TYPE_ALIADO: return VALUE_BOOL;
  case TYPE_UNKNOWN: return VALUE_NONE;
  }
  return VALUE_NONE;
}

static const char *kind_description(ValueKind kind) {
  switch (kind) {
  case VALUE_INT: return "numeros enteros";
  case VALUE_FLOAT: return "decimales";
  case VALUE_TEXT: return "texto";
  case VALUE_BOOL: return "paz ni guerra";
  case VALUE_NONE: return "ese valor";
  }
  return "ese valor";
}

static int accepts(DataType target, ValueKind kind) {
  switch (target) {
  case TYPE_RECURSO:
  case TYPE_EPOCA:
  case TYPE_MORAL: return kind == VALUE_INT;
  case TYPE_HEROE: return kind == VALUE_TEXT;
  case TYPE_ALIADO: return kind == VALUE_BOOL;
  case TYPE_UNKNOWN: return 0;
  }
  return 0;
}

static void check_value(DataType target, ASTNode *value, const SymbolTable *table) {
  if (value->type == IDENTIFIER_REF) {
    const Symbol *source = symbol_table_lookup(table, value->data.identifier);
    if (source == NULL) {
      defeat(value->line, "la unidad '%s' nunca fue reclutada", value->data.identifier);
      return;
    }
    value->valueKind = kind_of_type(source->type);
  }

  if (!accepts(target, value->valueKind)) {
    defeat(value->line, "%s %s no acepta %s", article(target), data_type_name(target),
           kind_description(value->valueKind));
    return;
  }

  if (target == TYPE_EPOCA && value->type == INT_LITERAL &&
      (value->data.intValue < 1 || value->data.intValue > 14)) {
    defeat(value->line, "solo existen 14 epocas (se puso %ld)", value->data.intValue);
  }

  if (target == TYPE_MORAL && value->type == INT_LITERAL &&
      (value->data.intValue < 0 || value->data.intValue > 5)) {
    defeat(value->line, "la moral solo va de 0 a 5 (se puso %ld)", value->data.intValue);
  }
}

int semantic_analysis(ASTNode *program, SymbolTable *table) {
  errors = 0;
  for (ASTNode *node = program; node != NULL; node = node->next) {
    if (node->type == VAR_DECL) {
      VarDeclaration *decl = &node->data.varDeclaration;
      if (decl->init != NULL) {
        check_value(decl->dataType, decl->init, table);
      }
      const Symbol *existing = symbol_table_lookup(table, decl->identifier);
      if (existing != NULL) {
        defeat(node->line, "la unidad '%s' ya fue reclutada en la linea %d", decl->identifier,
               existing->line);
      } else if (symbol_table_insert(table, decl->identifier, decl->dataType, node->line) < 0) {
        defeat(node->line, "no hay memoria para reclutar '%s'", decl->identifier);
      }
    } else if (node->type == ASSIGNMENT) {
      Assignment *assign = &node->data.assignment;
      const Symbol *target = symbol_table_lookup(table, assign->identifier);
      if (target == NULL) {
        defeat(node->line, "la unidad '%s' nunca fue reclutada", assign->identifier);
        continue;
      }
      check_value(target->type, assign->value, table);
    }
  }
  return errors;
}
