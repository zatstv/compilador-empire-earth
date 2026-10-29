#include "runtime.h"
#include "string_utils.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  ValueKind kind;
  SymbolValue data;
} RuntimeValue;

typedef struct RuntimeVariable {
  const char *name;
  DataType type;
  SymbolValue value;
  struct RuntimeVariable *next;
} RuntimeVariable;

typedef struct RuntimeArray {
  DataType elementType;
  SymbolValue *items;
  size_t length;
  size_t capacity;
  struct RuntimeArray *next_allocated;
} RuntimeArray;

static const ASTNode *program_root;
static SymbolTable *symbols;
static RuntimeVariable *global_variables;
static RuntimeArray *allocated_arrays;

static RuntimeArray *array_create(DataType element_type) {
  RuntimeArray *array = calloc(1, sizeof(*array));
  if (array == NULL) {
    fprintf(stderr, "Out of memory while creating array\n");
    return NULL;
  }
  array->elementType = element_type;
  array->next_allocated = allocated_arrays;
  allocated_arrays = array;
  return array;
}

static int grow_array(RuntimeArray *array) {
  if (array->capacity > SIZE_MAX / 2 / sizeof(*array->items)) {
    fprintf(stderr, "Array has reached its maximum capacity\n");
    return 0;
  }
  size_t new_capacity = array->capacity == 0 ? 4 : array->capacity * 2;
  SymbolValue *new_items = realloc(array->items, new_capacity * sizeof(*new_items));
  if (new_items == NULL) {
    fprintf(stderr, "Out of memory while growing array\n");
    return 0;
  }
  array->items = new_items;
  array->capacity = new_capacity;
  return 1;
}

static int array_push(RuntimeArray *array, SymbolValue value) {
  if (array->length == array->capacity && !grow_array(array)) return 0;
  array->items[array->length] = value;
  array->length++;
  return 1;
}

static void release_arrays(void) {
  while (allocated_arrays != NULL) {
    RuntimeArray *array = allocated_arrays;
    allocated_arrays = array->next_allocated;
    free(array->items);
    free(array);
  }
}

static RuntimeVariable *find_variable(RuntimeVariable *variables, const char *name) {
  for (; variables != NULL; variables = variables->next) {
    if (strcmp(variables->name, name) == 0) return variables;
  }
  return NULL;
}

static const ASTNode *find_function(const char *name) {
  for (const ASTNode *node = program_root; node != NULL; node = node->next) {
    if (node->type == FUNCTION_DECL &&
        strcmp(node->data.functionDeclaration.identifier, name) == 0) {
      return node;
    }
  }
  return NULL;
}

static void release_variables_until(RuntimeVariable **variables, RuntimeVariable *boundary) {
  while (*variables != boundary) {
    RuntimeVariable *next = (*variables)->next;
    free(*variables);
    *variables = next;
  }
}

static int push_variable(RuntimeVariable **variables, const char *name, DataType type,
                         SymbolValue value) {
  RuntimeVariable *variable = malloc(sizeof(*variable));
  if (variable == NULL) {
    fprintf(stderr, "Out of memory while creating runtime variable\n");
    return 0;
  }
  variable->name = name;
  variable->type = type;
  variable->value = value;
  variable->next = *variables;
  *variables = variable;
  return 1;
}

static SymbolValue default_value(DataType type) {
  SymbolValue value;
  value.intValue = 0;
  switch (type) {
  case TYPE_EPOCA: value.intValue = 1; break;
  case TYPE_HEROE: value.strValue = ""; break;
  case TYPE_RECURSO_ARRAY:
  case TYPE_EPOCA_ARRAY:
  case TYPE_MORAL_ARRAY:
  case TYPE_HEROE_ARRAY:
  case TYPE_ALIADO_ARRAY: value.arrayValue = array_create(array_element_type(type)); break;
  default: break;
  }
  return value;
}

static int fits(DataType type, const RuntimeValue *value, int line, const char *name) {
  long number = value->data.intValue;
  if (type == TYPE_EPOCA && (number < 1 || number > 14)) {
    fprintf(stderr, "DERROTA en la linea %d: solo existen 14 epocas ('%s' quedaria en %ld)\n",
            line, name, number);
    return 0;
  }
  if (type == TYPE_MORAL && (number < 0 || number > 5)) {
    fprintf(stderr, "DERROTA en la linea %d: la moral solo va de 0 a 5 ('%s' quedaria en %ld)\n",
            line, name, number);
    return 0;
  }
  if (type == TYPE_RECURSO && number < 0) {
    fprintf(stderr, "DERROTA en la linea %d: un recurso no puede quedar negativo ('%s' quedaria en %ld)\n",
            line, name, number);
    return 0;
  }
  return 1;
}

static int evaluate(const ASTNode *expr, RuntimeVariable *variables, RuntimeValue *result);
static int execute_statement(const ASTNode *node, RuntimeVariable **variables);
static int execute_if(const ASTNode *node, RuntimeVariable **variables, const ASTNode *call,
                      const FunctionDeclaration *function, RuntimeValue *result, int *returned);

static int bind_parameters(const FunctionDeclaration *function, const ASTNode *arguments,
                           RuntimeVariable *caller_variables, RuntimeVariable **variables) {
  const FunctionParameter *param = function->parameters;
  for (const ASTNode *arg = arguments; arg != NULL; arg = arg->next) {
    RuntimeValue value;
    if (!evaluate(arg, caller_variables, &value) ||
        !fits(param->dataType, &value, arg->line, param->identifier) ||
        !push_variable(variables, param->identifier, param->dataType, value.data)) {
      return 0;
    }
    param = param->next;
  }
  return 1;
}

static int execute_list(const ASTNode *node, RuntimeVariable **variables, const ASTNode *call,
                        const FunctionDeclaration *function, RuntimeValue *result,
                        int *returned) {
  for (; node != NULL; node = node->next) {
    if (node->type == RETURN_STMT) {
      *returned = 1;
      if (node->data.returnStatement.value == NULL) {
        return 1;
      }
      return evaluate(node->data.returnStatement.value, *variables, result) &&
             fits(function->returnType, result, call->line, function->identifier);
    }
    if (node->type == IF_STMT) {
      if (!execute_if(node, variables, call, function, result, returned)) return 0;
      if (*returned) return 1;
      continue;
    }
    if (!execute_statement(node, variables)) return 0;
  }
  return 1;
}

static int execute_if(const ASTNode *node, RuntimeVariable **variables, const ASTNode *call,
                      const FunctionDeclaration *function, RuntimeValue *result, int *returned) {
  RuntimeValue condition;
  if (!evaluate(node->data.ifStatement.condition, *variables, &condition)) return 0;
  const ASTNode *body = condition.data.boolValue ? node->data.ifStatement.thenBody
                                                 : node->data.ifStatement.elseBody;
  RuntimeVariable *block_start = *variables;
  int succeeded = execute_list(body, variables, call, function, result, returned);
  release_variables_until(variables, block_start);
  return succeeded;
}

static int execute_body(const ASTNode *call, const FunctionDeclaration *function,
                        RuntimeVariable **variables, RuntimeValue *result) {
  int returned = 0;
  return execute_list(function->body, variables, call, function, result, &returned);
}

static int call_function(const ASTNode *call, RuntimeVariable *caller_variables,
                         RuntimeValue *result) {
  const ASTNode *node = find_function(call->data.functionCall.identifier);
  const FunctionDeclaration *function = &node->data.functionDeclaration;
  RuntimeVariable *frame_start = global_variables;
  RuntimeVariable *variables = frame_start;
  result->kind = VALUE_NONE;
  int succeeded = bind_parameters(function, call->data.functionCall.arguments, caller_variables,
                                  &variables) &&
                  execute_body(call, function, &variables, result);
  release_variables_until(&variables, frame_start);
  return succeeded;
}

static int compare(BinaryOperator op, const RuntimeValue *left, const RuntimeValue *right) {
  long order;
  if (left->kind == VALUE_TEXT) {
    order = strcmp(left->data.strValue, right->data.strValue);
  } else if (left->kind == VALUE_BOOL) {
    order = left->data.boolValue - right->data.boolValue;
  } else {
    order = (left->data.intValue > right->data.intValue) -
            (left->data.intValue < right->data.intValue);
  }
  switch (op) {
  case OP_EQ: return order == 0;
  case OP_NE: return order != 0;
  case OP_GT: return order > 0;
  case OP_LT: return order < 0;
  case OP_GE: return order >= 0;
  case OP_LE: return order <= 0;
  default: return 0;
  }
}

static int evaluate_list(const ASTNode *expr, RuntimeVariable *variables, RuntimeValue *result) {
  DataType element = expr->data.arrayLiteral.elementType;
  RuntimeArray *array = array_create(element);
  if (array == NULL) return 0;
  for (const ASTNode *item = expr->data.arrayLiteral.elements; item != NULL; item = item->next) {
    RuntimeValue value;
    if (!evaluate(item, variables, &value) ||
        !fits(element, &value, item->line, "un elemento de la lista") ||
        !array_push(array, value.data)) {
      return 0;
    }
  }
  result->data.arrayValue = array;
  return 1;
}

static int evaluate_index(const ASTNode *expr, RuntimeVariable *variables, RuntimeValue *result) {
  const char *name = expr->data.arrayIndex.identifier;
  RuntimeArray *array = find_variable(variables, name)->value.arrayValue;
  RuntimeValue index;
  if (!evaluate(expr->data.arrayIndex.index, variables, &index)) return 0;
  if (index.data.intValue < 0 || (size_t)index.data.intValue >= array->length) {
    fprintf(stderr, "DERROTA en la linea %d: la lista '%s' no tiene la posicion %ld (tiene %zu elemento(s))\n",
            expr->line, name, index.data.intValue, array->length);
    return 0;
  }
  result->data = array->items[index.data.intValue];
  return 1;
}

static int evaluate_list_call(const ASTNode *expr, RuntimeVariable *variables,
                              RuntimeValue *result) {
  const char *name = expr->data.arrayMethodCall.identifier;
  RuntimeArray *array = find_variable(variables, name)->value.arrayValue;
  RuntimeValue value;
  switch (expr->data.arrayMethodCall.method) {
  case ARRAY_PUSH:
    return evaluate(expr->data.arrayMethodCall.arguments, variables, &value) &&
           fits(array->elementType, &value, expr->line, name) && array_push(array, value.data);
  case ARRAY_POP:
    if (array->length == 0) {
      fprintf(stderr, "DERROTA en la linea %d: la lista '%s' esta vacia, no hay nada que quitar\n",
              expr->line, name);
      return 0;
    }
    array->length--;
    result->data = array->items[array->length];
    return 1;
  case ARRAY_LENGTH:
    result->data.intValue = (long)array->length;
    return 1;
  case ARRAY_UNKNOWN_METHOD:
    return 0;
  }
  return 0;
}

static int evaluate(const ASTNode *expr, RuntimeVariable *variables, RuntimeValue *result) {
  result->kind = expr->valueKind;
  switch (expr->type) {
  case INT_LITERAL: result->data.intValue = expr->data.intValue; return 1;
  case STR_LITERAL: result->data.strValue = expr->data.strValue; return 1;
  case BOOL_LITERAL: result->data.boolValue = expr->data.boolValue; return 1;
  case IDENTIFIER_REF:
    result->data = find_variable(variables, expr->data.identifier)->value;
    return 1;
  case FUNCTION_CALL:
    return call_function(expr, variables, result);
  case ARRAY_LITERAL:
    return evaluate_list(expr, variables, result);
  case ARRAY_INDEX:
    return evaluate_index(expr, variables, result);
  case ARRAY_METHOD_CALL:
    return evaluate_list_call(expr, variables, result);
  case BINARY_OP: {
    RuntimeValue left;
    RuntimeValue right;
    if (!evaluate(expr->data.binary.left, variables, &left) ||
        !evaluate(expr->data.binary.right, variables, &right)) {
      return 0;
    }
    long a = left.data.intValue;
    long b = right.data.intValue;
    switch (expr->data.binary.op) {
    case OP_ADD: result->data.intValue = a + b; break;
    case OP_SUB: result->data.intValue = a - b; break;
    case OP_MUL: result->data.intValue = a * b; break;
    case OP_DIV:
      if (b == 0) {
        fprintf(stderr, "DERROTA en la linea %d: no se puede dividir entre 0\n", expr->line);
        return 0;
      }
      result->data.intValue = a / b;
      break;
    case OP_EQ:
    case OP_NE:
    case OP_GT:
    case OP_LT:
    case OP_GE:
    case OP_LE:
      result->data.boolValue = compare(expr->data.binary.op, &left, &right);
      break;
    }
    return 1;
  }
  default:
    return 0;
  }
}

static int execute_block(const ASTNode *node, RuntimeVariable *variables) {
  for (const ASTNode *item = node->data.block.body; item != NULL; item = item->next) {
    const ASTNode *amount = item->data.worldItem.amount;
    RuntimeValue value;
    if (amount == NULL) continue;
    const char *name = item->type == WATER_STMT ? "agua" : item->data.worldItem.word;
    if (!evaluate(amount, variables, &value) || !fits(TYPE_RECURSO, &value, amount->line, name)) {
      return 0;
    }
    for (Symbol *symbol = symbols->head; symbol != NULL; symbol = symbol->next) {
      if (symbol->item == item) {
        symbol->value = value.data;
        symbol->hasValue = 1;
      }
    }
  }
  return 1;
}

static int execute_statement(const ASTNode *node, RuntimeVariable **variables) {
  RuntimeValue value;
  switch (node->type) {
  case VAR_DECL: {
    const VarDeclaration *decl = &node->data.varDeclaration;
    if (decl->init == NULL) {
      value.data = default_value(decl->dataType);
      if (array_element_type(decl->dataType) != TYPE_UNKNOWN && value.data.arrayValue == NULL) {
        return 0;
      }
    } else if (!evaluate(decl->init, *variables, &value) ||
               !fits(decl->dataType, &value, decl->init->line, decl->identifier)) {
      return 0;
    }
    return push_variable(variables, decl->identifier, decl->dataType, value.data);
  }
  case ASSIGNMENT: {
    RuntimeVariable *target = find_variable(*variables, node->data.assignment.identifier);
    if (!evaluate(node->data.assignment.value, *variables, &value) ||
        !fits(target->type, &value, node->data.assignment.value->line, target->name)) {
      return 0;
    }
    target->value = value.data;
    return 1;
  }
  case FUNCTION_CALL:
    return call_function(node, *variables, &value);
  case ARRAY_METHOD_CALL:
    return evaluate(node, *variables, &value);
  case WORLD_DECL:
  case EMPIRE_DECL:
    return execute_block(node, *variables);
  case IF_STMT: {
    int returned = 0;
    return execute_if(node, variables, NULL, NULL, &value, &returned);
  }
  default:
    return 1;
  }
}

static void list_text(const RuntimeArray *array, char *buffer, size_t size) {
  size_t used = snprintf(buffer, size, "[");
  for (size_t i = 0; i < array->length && used < size; i++) {
    const SymbolValue *item = &array->items[i];
    const char *comma = i > 0 ? ", " : "";
    switch (array->elementType) {
    case TYPE_HEROE:
      used += snprintf(buffer + used, size - used, "%s\"%s\"", comma, item->strValue);
      break;
    case TYPE_ALIADO:
      used += snprintf(buffer + used, size - used, "%s%s", comma, item->boolValue ? "paz" : "guerra");
      break;
    default:
      used += snprintf(buffer + used, size - used, "%s%ld", comma, item->intValue);
      break;
    }
  }
  if (used + 2 > size) {
    snprintf(buffer, size, "(%zu elementos)", array->length);
  } else {
    buffer[used] = ']';
    buffer[used + 1] = '\0';
  }
}

static void save_globals(SymbolTable *table) {
  for (RuntimeVariable *variable = global_variables; variable != NULL; variable = variable->next) {
    Symbol *symbol = symbol_table_lookup(table, variable->name);
    if (symbol == NULL || symbol->hasValue) continue;
    symbol->value = variable->value;
    if (symbol->type == TYPE_HEROE) {
      symbol->value.strValue = string_copy(variable->value.strValue);
    } else if (array_element_type(symbol->type) != TYPE_UNKNOWN) {
      char text[64];
      list_text(variable->value.arrayValue, text, sizeof(text));
      symbol->value.strValue = string_copy(text);
    }
    symbol->hasValue = 1;
  }
}

int execute_program(const ASTNode *program, SymbolTable *table) {
  int succeeded = 1;
  program_root = program;
  symbols = table;
  global_variables = NULL;
  for (const ASTNode *node = program; node != NULL; node = node->next) {
    if (!execute_statement(node, &global_variables)) {
      succeeded = 0;
      break;
    }
  }
  save_globals(table);
  release_variables_until(&global_variables, NULL);
  release_arrays();
  return succeeded;
}
