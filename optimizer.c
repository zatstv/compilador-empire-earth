#include "optimizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int changes;

static void literal_text(const ASTNode *node, char *buffer, size_t size) {
  switch (node->type) {
  case INT_LITERAL: snprintf(buffer, size, "%ld", node->data.intValue); break;
  case BOOL_LITERAL: snprintf(buffer, size, "%s", node->data.boolValue ? "paz" : "guerra"); break;
  case STR_LITERAL: snprintf(buffer, size, "\"%s\"", node->data.strValue); break;
  default: snprintf(buffer, size, "?"); break;
  }
}

static int is_literal(const ASTNode *node) {
  return node != NULL &&
         (node->type == INT_LITERAL || node->type == BOOL_LITERAL || node->type == STR_LITERAL);
}

static int compare(BinaryOperator op, long order) {
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

static void fold_expression(ASTNode *expr);

static void fold_list(ASTNode *expr) {
  for (; expr != NULL; expr = expr->next) {
    fold_expression(expr);
  }
}

static void fold_binary(ASTNode *expr) {
  ASTNode *left = expr->data.binary.left;
  ASTNode *right = expr->data.binary.right;
  BinaryOperator op = expr->data.binary.op;
  if (!is_literal(left) || !is_literal(right) || left->type != right->type) return;

  long number = 0;
  int boolean = 0;
  NodeType result;
  if (left->type == INT_LITERAL) {
    long a = left->data.intValue;
    long b = right->data.intValue;
    result = INT_LITERAL;
    switch (op) {
    case OP_ADD: number = a + b; break;
    case OP_SUB: number = a - b; break;
    case OP_MUL: number = a * b; break;
    case OP_DIV:
      if (b == 0) return;
      number = a / b;
      break;
    default:
      result = BOOL_LITERAL;
      boolean = compare(op, (a > b) - (a < b));
      break;
    }
  } else if (op == OP_EQ || op == OP_NE) {
    long order = left->type == BOOL_LITERAL
                     ? left->data.boolValue - right->data.boolValue
                     : strcmp(left->data.strValue, right->data.strValue);
    result = BOOL_LITERAL;
    boolean = compare(op, order);
  } else {
    return;
  }

  char a_text[128], b_text[128];
  literal_text(left, a_text, sizeof a_text);
  literal_text(right, b_text, sizeof b_text);
  free_ast(left);
  free_ast(right);
  expr->type = result;
  if (result == INT_LITERAL) {
    expr->valueKind = VALUE_INT;
    expr->data.intValue = number;
  } else {
    expr->valueKind = VALUE_BOOL;
    expr->data.boolValue = boolean;
  }
  char r_text[128];
  literal_text(expr, r_text, sizeof r_text);
  printf("  linea %-3d %s %s %s  ->  %s\n", expr->line, a_text, binary_operator_name(op), b_text,
         r_text);
  changes++;
}

static void fold_expression(ASTNode *expr) {
  if (expr == NULL) return;
  switch (expr->type) {
  case BINARY_OP:
    fold_expression(expr->data.binary.left);
    fold_expression(expr->data.binary.right);
    fold_binary(expr);
    break;
  case FUNCTION_CALL: fold_list(expr->data.functionCall.arguments); break;
  case ARRAY_LITERAL: fold_list(expr->data.arrayLiteral.elements); break;
  case ARRAY_INDEX: fold_expression(expr->data.arrayIndex.index); break;
  case ARRAY_METHOD_CALL: fold_list(expr->data.arrayMethodCall.arguments); break;
  default: break;
  }
}

static void optimize_list(ASTNode **list);

static int remove_statement(ASTNode *node) {
  if (node->type == IF_STMT) {
    ASTNode *condition = node->data.ifStatement.condition;
    if (condition == NULL || condition->type != BOOL_LITERAL) return 0;
    if (condition->data.boolValue) {
      if (node->data.ifStatement.elseBody != NULL) {
        printf("  linea %-3d el si siempre se cumple, se quito el sino\n", node->line);
        free_ast(node->data.ifStatement.elseBody);
        node->data.ifStatement.elseBody = NULL;
        changes++;
      }
      return 0;
    }
    if (node->data.ifStatement.elseBody == NULL) {
      printf("  linea %-3d el si nunca se cumple, se quito\n", node->line);
      changes++;
      return 1;
    }
    printf("  linea %-3d el si nunca se cumple, solo queda el sino\n", node->line);
    free_ast(node->data.ifStatement.thenBody);
    node->data.ifStatement.thenBody = node->data.ifStatement.elseBody;
    node->data.ifStatement.elseBody = NULL;
    condition->data.boolValue = 1;
    changes++;
    return 0;
  }
  if (node->type == WHILE_STMT) {
    ASTNode *condition = node->data.whileStatement.condition;
    if (condition != NULL && condition->type == BOOL_LITERAL && !condition->data.boolValue) {
      printf("  linea %-3d el mientras nunca se cumple, se quito\n", node->line);
      changes++;
      return 1;
    }
  }
  return 0;
}

static void optimize_statement(ASTNode *node) {
  switch (node->type) {
  case VAR_DECL: fold_expression(node->data.varDeclaration.init); break;
  case ASSIGNMENT: fold_expression(node->data.assignment.value); break;
  case RETURN_STMT: fold_expression(node->data.returnStatement.value); break;
  case FUNCTION_CALL:
  case ARRAY_METHOD_CALL: fold_expression(node); break;
  case FUNCTION_DECL: optimize_list(&node->data.functionDeclaration.body); break;
  case WORLD_DECL:
  case EMPIRE_DECL: optimize_list(&node->data.block.body); break;
  case EPOCH_BLOCK:
    fold_expression(node->data.epochBlock.epoch);
    optimize_list(&node->data.epochBlock.body);
    break;
  case SIZE_STMT:
  case TERRAIN_STMT:
  case WATER_STMT:
  case POPULATE_STMT:
  case TREASURE_STMT: fold_expression(node->data.worldItem.amount); break;
  case IF_STMT:
    fold_expression(node->data.ifStatement.condition);
    optimize_list(&node->data.ifStatement.thenBody);
    optimize_list(&node->data.ifStatement.elseBody);
    break;
  case WHILE_STMT:
    fold_expression(node->data.whileStatement.condition);
    optimize_list(&node->data.whileStatement.body);
    break;
  default: break;
  }
}

static void optimize_list(ASTNode **list) {
  ASTNode **link = list;
  while (*link != NULL) {
    ASTNode *node = *link;
    optimize_statement(node);
    if (remove_statement(node)) {
      *link = node->next;
      node->next = NULL;
      free_ast(node);
    } else {
      link = &node->next;
    }
  }
}

int optimize_program(ASTNode **program) {
  changes = 0;
  optimize_list(program);
  return changes;
}
