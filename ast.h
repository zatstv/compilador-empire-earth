#ifndef AST_H
#define AST_H

typedef enum {
  TYPE_RECURSO,
  TYPE_EPOCA,
  TYPE_MORAL,
  TYPE_HEROE,
  TYPE_ALIADO,
  TYPE_UNKNOWN
} DataType;

typedef enum {
  VALUE_INT,
  VALUE_FLOAT,
  VALUE_TEXT,
  VALUE_BOOL,
  VALUE_NONE
} ValueKind;

typedef enum {
  VAR_DECL,
  ASSIGNMENT,

  INT_LITERAL,
  FLOAT_LITERAL,
  STR_LITERAL,
  BOOL_LITERAL,
  IDENTIFIER_REF,
} NodeType;

typedef struct VarDeclaration {
  DataType dataType;
  char *identifier;
  struct ASTNode *init;
} VarDeclaration;

typedef struct Assignment {
  char *identifier;
  struct ASTNode *value;
} Assignment;

typedef struct ASTNode {
  NodeType type;
  int line;
  ValueKind valueKind;
  union {
    VarDeclaration varDeclaration;
    Assignment assignment;
    long intValue;
    double floatValue;
    char *strValue;
    int boolValue;
    char *identifier;
  } data;

  struct ASTNode *next; // Linked list for multiple statements

} ASTNode;

ASTNode *create_int_node(long val, int line);
ASTNode *create_float_node(double val, int line);
ASTNode *create_str_node(const char *val, int line);
ASTNode *create_bool_node(int val, int line);
ASTNode *create_identifier_node(const char *name, int line);
ASTNode *create_var_decl(DataType dtype, const char *name, ASTNode *init, int line);
ASTNode *create_assignment(const char *name, ASTNode *value, int line);
ASTNode *append_statement(ASTNode *list, ASTNode *statement);

const char *data_type_name(DataType type);
const char *value_kind_name(ValueKind kind);
void print_ast_table(const ASTNode *root);
void free_ast(ASTNode *node);

#endif
