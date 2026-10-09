#include "runtime.h"
#include "string_utils.h"

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

static const ASTNode *program_root;
static RuntimeVariable *global_variables;

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

static int execute_body(const ASTNode *call, const FunctionDeclaration *function,
                        RuntimeVariable **variables, RuntimeValue *result) {
  for (const ASTNode *node = function->body; node != NULL; node = node->next) {
    if (node->type == RETURN_STMT) {
      if (node->data.returnStatement.value == NULL) {
        return 1;
      }
      return evaluate(node->data.returnStatement.value, *variables, result) &&
             fits(function->returnType, result, call->line, function->identifier);
    }
    if (!execute_statement(node, variables)) return 0;
  }
  return 1;
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
    }
    return 1;
  }
  default:
    return 0;
  }
}

static int execute_statement(const ASTNode *node, RuntimeVariable **variables) {
  RuntimeValue value;
  switch (node->type) {
  case VAR_DECL: {
    const VarDeclaration *decl = &node->data.varDeclaration;
    value.data = default_value(decl->dataType);
    if (decl->init != NULL &&
        (!evaluate(decl->init, *variables, &value) ||
         !fits(decl->dataType, &value, decl->init->line, decl->identifier))) {
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
  default:
    return 1;
  }
}

static void save_globals(SymbolTable *table) {
  for (RuntimeVariable *variable = global_variables; variable != NULL; variable = variable->next) {
    Symbol *symbol = symbol_table_lookup(table, variable->name);
    if (symbol == NULL || symbol->hasValue) continue;
    symbol->value = variable->value;
    if (symbol->type == TYPE_HEROE) {
      symbol->value.strValue = string_copy(variable->value.strValue);
    }
    symbol->hasValue = 1;
  }
}

int execute_program(const ASTNode *program, SymbolTable *table) {
  int succeeded = 1;
  program_root = program;
  global_variables = NULL;
  for (const ASTNode *node = program; node != NULL; node = node->next) {
    if (!execute_statement(node, &global_variables)) {
      succeeded = 0;
      break;
    }
  }
  save_globals(table);
  release_variables_until(&global_variables, NULL);
  return succeeded;
}
