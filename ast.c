#include "ast.h"
#include "string_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static ASTNode *allocate_new_node(NodeType type, int line) {
  ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));

  // If fails in locate in memory
  if (node == NULL) {
    fprintf(stderr, "Out of memory\n");
    exit(EXIT_FAILURE);
  }
  node->type = type;
  node->line = line;
  node->valueKind = VALUE_NONE;
  node->next = NULL;
  return node;
}

ASTNode *create_int_node(long val, int line) {
  ASTNode *node = allocate_new_node(INT_LITERAL, line);
  node->valueKind = VALUE_INT;
  node->data.intValue = val;
  return node;
}

ASTNode *create_float_node(double val, int line) {
  ASTNode *node = allocate_new_node(FLOAT_LITERAL, line);
  node->valueKind = VALUE_FLOAT;
  node->data.floatValue = val;
  return node;
}

ASTNode *create_str_node(const char *val, int line) {
  ASTNode *node = allocate_new_node(STR_LITERAL, line);
  node->valueKind = VALUE_TEXT;
  node->data.strValue = string_copy(val);
  return node;
}

ASTNode *create_bool_node(int val, int line) {
  ASTNode *node = allocate_new_node(BOOL_LITERAL, line);
  node->valueKind = VALUE_BOOL;
  node->data.boolValue = val;
  return node;
}

ASTNode *create_identifier_node(const char *name, int line) {
  ASTNode *node = allocate_new_node(IDENTIFIER_REF, line);
  node->data.identifier = string_copy(name);
  return node;
}

ASTNode *create_var_decl(DataType dtype, const char *name, ASTNode *init, int line) {
  ASTNode *node = allocate_new_node(VAR_DECL, line);
  node->data.varDeclaration.dataType = dtype;
  node->data.varDeclaration.identifier = string_copy(name);
  node->data.varDeclaration.init = init;
  return node;
}

ASTNode *create_assignment(const char *name, ASTNode *value, int line) {
  ASTNode *node = allocate_new_node(ASSIGNMENT, line);
  node->data.assignment.identifier = string_copy(name);
  node->data.assignment.value = value;
  return node;
}

ASTNode *append_statement(ASTNode *list, ASTNode *statement) {
  if (list == NULL) {
    return statement;
  }
  ASTNode *cur = list;
  while (cur->next != NULL) {
    cur = cur->next;
  }
  cur->next = statement;
  return list;
}

FunctionParameter *create_function_parameter(DataType dtype, const char *name, int line) {
  FunctionParameter *parameter = malloc(sizeof(*parameter));
  if (parameter == NULL) {
    fprintf(stderr, "Out of memory\n");
    exit(EXIT_FAILURE);
  }
  parameter->dataType = dtype;
  parameter->identifier = string_copy(name);
  parameter->line = line;
  parameter->next = NULL;
  return parameter;
}

ASTNode *create_function_decl(DataType return_type, const char *name,
                              FunctionParameter *parameters, ASTNode *body, int line) {
  ASTNode *node = allocate_new_node(FUNCTION_DECL, line);
  node->data.functionDeclaration.returnType = return_type;
  node->data.functionDeclaration.identifier = string_copy(name);
  node->data.functionDeclaration.parameters = parameters;
  node->data.functionDeclaration.body = body;
  return node;
}

ASTNode *create_function_call(const char *name, ASTNode *arguments, int line) {
  ASTNode *node = allocate_new_node(FUNCTION_CALL, line);
  node->data.functionCall.identifier = string_copy(name);
  node->data.functionCall.arguments = arguments;
  return node;
}

ASTNode *create_return_node(ASTNode *value, int line) {
  ASTNode *node = allocate_new_node(RETURN_STMT, line);
  node->data.returnStatement.value = value;
  return node;
}

ASTNode *create_binary_node(BinaryOperator op, ASTNode *left, ASTNode *right, int line) {
  ASTNode *node = allocate_new_node(BINARY_OP, line);
  node->data.binary.op = op;
  node->data.binary.left = left;
  node->data.binary.right = right;
  return node;
}

const char *data_type_name(DataType type) {
  switch (type) {
  case TYPE_RECURSO: return "recurso";
  case TYPE_EPOCA: return "epoca";
  case TYPE_MORAL: return "moral";
  case TYPE_HEROE: return "heroe";
  case TYPE_ALIADO: return "aliado";
  case TYPE_NADA: return "nada";
  case TYPE_UNKNOWN: return "?";
  }
  return "?";
}

const char *value_kind_name(ValueKind kind) {
  switch (kind) {
  case VALUE_INT: return "entero";
  case VALUE_FLOAT: return "decimal";
  case VALUE_TEXT: return "texto";
  case VALUE_BOOL: return "paz/guerra";
  case VALUE_NONE: return "-";
  }
  return "-";
}

const char *binary_operator_name(BinaryOperator op) {
  switch (op) {
  case OP_ADD: return "+";
  case OP_SUB: return "-";
  case OP_MUL: return "*";
  case OP_DIV: return "/";
  }
  return "?";
}

static int row_number;

static void table_line(void) {
  printf("+-----+-------+--------------------------------+------------+--------------+------------------+\n");
}

static void table_row(int line, int depth, const char *node, const char *type, const char *name,
                      const char *value) {
  char text[64];
  snprintf(text, sizeof(text), "%*s%s%s", depth * 2, "", depth > 0 ? "+- " : "", node);
  printf("| %-3d | %-5d | %-30s | %-10s | %-12s | %-16s |\n", ++row_number, line, text,
         type, name, value);
}

static void expression_text(const ASTNode *expr, char *buffer, size_t size) {
  switch (expr->type) {
  case INT_LITERAL: snprintf(buffer, size, "%ld", expr->data.intValue); break;
  case FLOAT_LITERAL: snprintf(buffer, size, "%g", expr->data.floatValue); break;
  case STR_LITERAL: snprintf(buffer, size, "\"%s\"", expr->data.strValue); break;
  case BOOL_LITERAL: snprintf(buffer, size, "%s", expr->data.boolValue ? "paz" : "guerra"); break;
  case IDENTIFIER_REF: snprintf(buffer, size, "%s", expr->data.identifier); break;
  case BINARY_OP: snprintf(buffer, size, "%s", binary_operator_name(expr->data.binary.op)); break;
  default: snprintf(buffer, size, "-"); break;
  }
}

static void expression_rows(const ASTNode *expr, int depth) {
  char value[64];
  const char *node = "?";
  const char *name = "-";
  switch (expr->type) {
  case INT_LITERAL: node = "Literal entero"; break;
  case FLOAT_LITERAL: node = "Literal decimal"; break;
  case STR_LITERAL: node = "Literal texto"; break;
  case BOOL_LITERAL: node = "Literal paz/guerra"; break;
  case IDENTIFIER_REF: node = "Uso de variable"; name = expr->data.identifier; break;
  case BINARY_OP: node = "Operacion"; break;
  case FUNCTION_CALL: node = "Llamada a estrategia"; name = expr->data.functionCall.identifier; break;
  default: break;
  }
  expression_text(expr, value, sizeof(value));
  table_row(expr->line, depth, node, value_kind_name(expr->valueKind), name, value);
  if (expr->type == BINARY_OP) {
    expression_rows(expr->data.binary.left, depth + 1);
    expression_rows(expr->data.binary.right, depth + 1);
  } else if (expr->type == FUNCTION_CALL) {
    for (const ASTNode *arg = expr->data.functionCall.arguments; arg != NULL; arg = arg->next) {
      expression_rows(arg, depth + 1);
    }
  }
}

static void statement_rows(const ASTNode *node, int depth) {
  for (; node != NULL; node = node->next) {
    if (node->type == VAR_DECL) {
      const VarDeclaration *decl = &node->data.varDeclaration;
      table_row(node->line, depth, "Declaracion (reclutar)", data_type_name(decl->dataType),
                decl->identifier, decl->init ? "" : "(por defecto)");
      if (decl->init) {
        expression_rows(decl->init, depth + 1);
      }
    } else if (node->type == ASSIGNMENT) {
      table_row(node->line, depth, "Asignacion (<-)", "-", node->data.assignment.identifier, "");
      expression_rows(node->data.assignment.value, depth + 1);
    } else if (node->type == FUNCTION_DECL) {
      const FunctionDeclaration *function = &node->data.functionDeclaration;
      table_row(node->line, depth, "Estrategia (funcion)", data_type_name(function->returnType),
                function->identifier, "");
      for (const FunctionParameter *param = function->parameters; param != NULL;
           param = param->next) {
        table_row(param->line, depth + 1, "Parametro", data_type_name(param->dataType),
                  param->identifier, "");
      }
      statement_rows(function->body, depth + 1);
    } else if (node->type == RETURN_STMT) {
      table_row(node->line, depth, "Tributo (retorno)", "-", "-", "");
      if (node->data.returnStatement.value) {
        expression_rows(node->data.returnStatement.value, depth + 1);
      }
    } else if (node->type == FUNCTION_CALL) {
      expression_rows(node, depth);
    }
  }
}

void print_ast_table(const ASTNode *node) {
  row_number = 0;
  table_line();
  printf("| %-3s | %-5s | %-30s | %-10s | %-12s | %-16s |\n", "#", "Linea", "Nodo", "Tipo",
         "Nombre", "Valor");
  table_line();
  statement_rows(node, 0);
  table_line();
}

void free_parameters(FunctionParameter *parameter) {
  while (parameter != NULL) {
    FunctionParameter *next_parameter = parameter->next;
    free(parameter->identifier);
    free(parameter);
    parameter = next_parameter;
  }
}

void free_ast(ASTNode *node) {
  while (node != NULL) {
    /* Save next statement before freeing current node */
    ASTNode *next_node = node->next;

    switch (node->type) {
    case VAR_DECL:
      free(node->data.varDeclaration.identifier);
      free_ast(node->data.varDeclaration.init);
      break;
    case ASSIGNMENT:
      free(node->data.assignment.identifier);
      free_ast(node->data.assignment.value);
      break;
    case STR_LITERAL:
      free(node->data.strValue);
      break;
    case IDENTIFIER_REF:
      free(node->data.identifier);
      break;
    case FUNCTION_DECL:
      free(node->data.functionDeclaration.identifier);
      free_parameters(node->data.functionDeclaration.parameters);
      free_ast(node->data.functionDeclaration.body);
      break;
    case FUNCTION_CALL:
      free(node->data.functionCall.identifier);
      free_ast(node->data.functionCall.arguments);
      break;
    case RETURN_STMT:
      free_ast(node->data.returnStatement.value);
      break;
    case BINARY_OP:
      free_ast(node->data.binary.left);
      free_ast(node->data.binary.right);
      break;
    case INT_LITERAL:
    case FLOAT_LITERAL:
    case BOOL_LITERAL:
      /* No dynamically allocated fields */
      break;
    }

    /* Free the ASTNode struct itself */
    free(node);

    /* Advance to the next statement in the linked list */
    node = next_node;
  }
}
