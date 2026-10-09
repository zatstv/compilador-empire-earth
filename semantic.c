#include "semantic.h"

#include <stdarg.h>
#include <stdio.h>

static int errors;
static const FunctionDeclaration *current_function;

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
  case TYPE_NADA:
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
  case TYPE_NADA:
  case TYPE_UNKNOWN: return 0;
  }
  return 0;
}

static ValueKind check_expression(ASTNode *expr, SymbolTable *table);

static void check_range(DataType target, const ASTNode *value) {
  if (target == TYPE_EPOCA && value->type == INT_LITERAL &&
      (value->data.intValue < 1 || value->data.intValue > 14)) {
    defeat(value->line, "solo existen 14 epocas (se puso %ld)", value->data.intValue);
  }

  if (target == TYPE_MORAL && value->type == INT_LITERAL &&
      (value->data.intValue < 0 || value->data.intValue > 5)) {
    defeat(value->line, "la moral solo va de 0 a 5 (se puso %ld)", value->data.intValue);
  }
}

static void check_value(DataType target, ASTNode *value, SymbolTable *table) {
  ValueKind kind = check_expression(value, table);
  if (kind == VALUE_NONE) {
    return;
  }

  if (!accepts(target, kind)) {
    defeat(value->line, "%s %s no acepta %s", article(target), data_type_name(target),
           kind_description(kind));
    return;
  }

  check_range(target, value);
}

static DataType check_call(ASTNode *call, SymbolTable *table) {
  const char *name = call->data.functionCall.identifier;
  const Symbol *symbol = symbol_table_lookup(table, name);
  if (symbol == NULL) {
    defeat(call->line, "la estrategia '%s' no existe (hay que crearla antes de usarla)", name);
    return TYPE_UNKNOWN;
  }
  if (symbol->kind != SYMBOL_FUNCTION) {
    defeat(call->line, "'%s' es una unidad, no una estrategia", name);
    return TYPE_UNKNOWN;
  }

  const FunctionDeclaration *function = &symbol->function->data.functionDeclaration;
  const FunctionParameter *param = function->parameters;
  ASTNode *arg = call->data.functionCall.arguments;
  int params = 0;
  int args = 0;
  for (; param != NULL; param = param->next) params++;
  for (; arg != NULL; arg = arg->next) args++;
  if (params != args) {
    defeat(call->line, "la estrategia '%s' pide %d parametro(s) y se le dieron %d", name,
           params, args);
    return function->returnType;
  }

  param = function->parameters;
  for (arg = call->data.functionCall.arguments; arg != NULL; arg = arg->next) {
    ValueKind kind = check_expression(arg, table);
    if (kind != VALUE_NONE && !accepts(param->dataType, kind)) {
      defeat(arg->line, "el parametro '%s' de '%s' es %s y no acepta %s", param->identifier,
             name, data_type_name(param->dataType), kind_description(kind));
    } else if (kind != VALUE_NONE) {
      check_range(param->dataType, arg);
    }
    param = param->next;
  }
  return function->returnType;
}

static ValueKind check_expression(ASTNode *expr, SymbolTable *table) {
  switch (expr->type) {
  case IDENTIFIER_REF: {
    const Symbol *source = symbol_table_lookup(table, expr->data.identifier);
    if (source == NULL) {
      defeat(expr->line, "la unidad '%s' nunca fue reclutada", expr->data.identifier);
      return VALUE_NONE;
    }
    if (source->kind == SYMBOL_FUNCTION) {
      defeat(expr->line, "'%s' es una estrategia, se usa con ( )", expr->data.identifier);
      return VALUE_NONE;
    }
    expr->valueKind = kind_of_type(source->type);
    break;
  }
  case FUNCTION_CALL: {
    DataType type = check_call(expr, table);
    if (type == TYPE_NADA) {
      defeat(expr->line, "la estrategia '%s' es de tipo nada y no entrega tributo",
             expr->data.functionCall.identifier);
    }
    expr->valueKind = kind_of_type(type);
    break;
  }
  case BINARY_OP: {
    ValueKind left = check_expression(expr->data.binary.left, table);
    ValueKind right = check_expression(expr->data.binary.right, table);
    if (left == VALUE_NONE || right == VALUE_NONE) {
      return VALUE_NONE;
    }
    if (left != VALUE_INT || right != VALUE_INT) {
      defeat(expr->line, "el %s solo se puede hacer con numeros enteros",
             binary_operator_name(expr->data.binary.op));
      return VALUE_NONE;
    }
    expr->valueKind = VALUE_INT;
    break;
  }
  default:
    break;
  }
  return expr->valueKind;
}

static int has_return(const ASTNode *body) {
  for (; body != NULL; body = body->next) {
    if (body->type == RETURN_STMT) return 1;
  }
  return 0;
}

static void check_statement(ASTNode *node, SymbolTable *table);

static void check_function(ASTNode *node, SymbolTable *table) {
  const FunctionDeclaration *function = &node->data.functionDeclaration;
  const Symbol *existing = symbol_table_lookup_current(table, function->identifier);
  if (existing != NULL) {
    defeat(node->line, "'%s' ya existe desde la linea %d", function->identifier, existing->line);
    return;
  }
  Symbol *symbol = symbol_table_insert(table, function->identifier, function->returnType,
                                       SYMBOL_FUNCTION, node->line);
  if (symbol == NULL) {
    defeat(node->line, "no hay memoria para la estrategia '%s'", function->identifier);
    return;
  }
  symbol->function = node;
  symbol->visible = 0;

  symbol_table_enter_scope(table, function->identifier);
  for (const FunctionParameter *param = function->parameters; param != NULL;
       param = param->next) {
    const Symbol *repeated = symbol_table_lookup_current(table, param->identifier);
    if (repeated != NULL) {
      defeat(param->line, "el parametro '%s' esta repetido", param->identifier);
    } else if (symbol_table_insert(table, param->identifier, param->dataType, SYMBOL_PARAMETER,
                                   param->line) == NULL) {
      defeat(param->line, "no hay memoria para el parametro '%s'", param->identifier);
    }
  }

  current_function = function;
  for (ASTNode *statement = function->body; statement != NULL; statement = statement->next) {
    check_statement(statement, table);
  }
  current_function = NULL;

  if (function->returnType != TYPE_NADA && !has_return(function->body)) {
    defeat(node->line, "la estrategia '%s' tiene que entregar tributo de tipo %s",
           function->identifier, data_type_name(function->returnType));
  }
  symbol_table_exit_scope(table);
  symbol->visible = 1;
}

static void check_statement(ASTNode *node, SymbolTable *table) {
  if (node->type == VAR_DECL) {
    VarDeclaration *decl = &node->data.varDeclaration;
    if (decl->init != NULL) {
      check_value(decl->dataType, decl->init, table);
    }
    const Symbol *existing = symbol_table_lookup_current(table, decl->identifier);
    if (existing != NULL) {
      defeat(node->line, "la unidad '%s' ya fue reclutada en la linea %d", decl->identifier,
             existing->line);
    } else if (symbol_table_insert(table, decl->identifier, decl->dataType, SYMBOL_UNIT,
                                   node->line) == NULL) {
      defeat(node->line, "no hay memoria para reclutar '%s'", decl->identifier);
    }
  } else if (node->type == ASSIGNMENT) {
    Assignment *assign = &node->data.assignment;
    const Symbol *target = symbol_table_lookup(table, assign->identifier);
    if (target == NULL) {
      defeat(node->line, "la unidad '%s' nunca fue reclutada", assign->identifier);
      return;
    }
    if (target->kind == SYMBOL_FUNCTION) {
      defeat(node->line, "'%s' es una estrategia y no se le puede asignar", assign->identifier);
      return;
    }
    check_value(target->type, assign->value, table);
  } else if (node->type == FUNCTION_DECL) {
    check_function(node, table);
  } else if (node->type == FUNCTION_CALL) {
    check_call(node, table);
  } else if (node->type == RETURN_STMT) {
    ASTNode *value = node->data.returnStatement.value;
    if (value == NULL && current_function->returnType != TYPE_NADA) {
      defeat(node->line, "la estrategia '%s' tiene que entregar tributo de tipo %s",
             current_function->identifier, data_type_name(current_function->returnType));
    } else if (value != NULL && current_function->returnType == TYPE_NADA) {
      defeat(node->line, "la estrategia '%s' es de tipo nada y no entrega tributo",
             current_function->identifier);
    } else if (value != NULL) {
      check_value(current_function->returnType, value, table);
    }
  }
}

int semantic_analysis(ASTNode *program, SymbolTable *table) {
  errors = 0;
  current_function = NULL;
  for (ASTNode *node = program; node != NULL; node = node->next) {
    check_statement(node, table);
  }
  return errors;
}
