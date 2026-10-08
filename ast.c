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

const char *data_type_name(DataType type) {
  switch (type) {
  case TYPE_RECURSO: return "recurso";
  case TYPE_EPOCA: return "epoca";
  case TYPE_MORAL: return "moral";
  case TYPE_HEROE: return "heroe";
  case TYPE_ALIADO: return "aliado";
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

static int row_number;

static void table_line(void) {
  printf("+-----+-------+--------------------------+------------+--------------+------------------+\n");
}

static void table_row(int line, const char *node, const char *type, const char *name,
                      const char *value) {
  printf("| %-3d | %-5d | %-24s | %-10s | %-12s | %-16s |\n", ++row_number, line, node,
         type, name, value);
}

static void expression_text(const ASTNode *expr, char *buffer, size_t size) {
  switch (expr->type) {
  case INT_LITERAL: snprintf(buffer, size, "%ld", expr->data.intValue); break;
  case FLOAT_LITERAL: snprintf(buffer, size, "%g", expr->data.floatValue); break;
  case STR_LITERAL: snprintf(buffer, size, "\"%s\"", expr->data.strValue); break;
  case BOOL_LITERAL: snprintf(buffer, size, "%s", expr->data.boolValue ? "paz" : "guerra"); break;
  case IDENTIFIER_REF: snprintf(buffer, size, "%s", expr->data.identifier); break;
  default: snprintf(buffer, size, "-"); break;
  }
}

static void expression_row(const ASTNode *expr) {
  char value[64];
  const char *node = "  +- ?";
  switch (expr->type) {
  case INT_LITERAL: node = "  +- Literal entero"; break;
  case FLOAT_LITERAL: node = "  +- Literal decimal"; break;
  case STR_LITERAL: node = "  +- Literal texto"; break;
  case BOOL_LITERAL: node = "  +- Literal paz/guerra"; break;
  case IDENTIFIER_REF: node = "  +- Uso de variable"; break;
  default: break;
  }
  expression_text(expr, value, sizeof(value));
  table_row(expr->line, node, value_kind_name(expr->valueKind),
            expr->type == IDENTIFIER_REF ? expr->data.identifier : "-", value);
}

void print_ast_table(const ASTNode *node) {
  row_number = 0;
  table_line();
  printf("| %-3s | %-5s | %-24s | %-10s | %-12s | %-16s |\n", "#", "Linea", "Nodo", "Tipo",
         "Nombre", "Valor");
  table_line();
  while (node != NULL) {
    if (node->type == VAR_DECL) {
      const VarDeclaration *decl = &node->data.varDeclaration;
      table_row(node->line, "Declaracion (reclutar)", data_type_name(decl->dataType),
                decl->identifier, decl->init ? "" : "(por defecto)");
      if (decl->init) {
        expression_row(decl->init);
      }
    } else if (node->type == ASSIGNMENT) {
      table_row(node->line, "Asignacion (<-)", "-", node->data.assignment.identifier, "");
      expression_row(node->data.assignment.value);
    }
    node = node->next;
  }
  table_line();
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
