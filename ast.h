#ifndef AST_H
#define AST_H

typedef enum {
  TYPE_RECURSO,
  TYPE_EPOCA,
  TYPE_MORAL,
  TYPE_HEROE,
  TYPE_ALIADO,
  TYPE_NADA,
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
  OP_ADD,
  OP_SUB,
  OP_MUL,
  OP_DIV,
} BinaryOperator;

typedef enum {
  VAR_DECL,
  ASSIGNMENT,
  FUNCTION_DECL,
  RETURN_STMT,

  INT_LITERAL,
  FLOAT_LITERAL,
  STR_LITERAL,
  BOOL_LITERAL,
  IDENTIFIER_REF,
  FUNCTION_CALL,
  BINARY_OP,
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

typedef struct FunctionParameter {
  DataType dataType;
  char *identifier;
  int line;
  struct FunctionParameter *next;
} FunctionParameter;

typedef struct FunctionDeclaration {
  DataType returnType;
  char *identifier;
  FunctionParameter *parameters;
  struct ASTNode *body;
} FunctionDeclaration;

typedef struct ASTNode {
  NodeType type;
  int line;
  ValueKind valueKind;
  union {
    VarDeclaration varDeclaration;
    Assignment assignment;
    FunctionDeclaration functionDeclaration;
    long intValue;
    double floatValue;
    char *strValue;
    int boolValue;
    char *identifier;
    struct {
      char *identifier;
      struct ASTNode *arguments;
    } functionCall;
    struct {
      BinaryOperator op;
      struct ASTNode *left;
      struct ASTNode *right;
    } binary;
    struct {
      struct ASTNode *value;
    } returnStatement;
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
FunctionParameter *create_function_parameter(DataType dtype, const char *name, int line);
ASTNode *create_function_decl(DataType return_type, const char *name,
                              FunctionParameter *parameters, ASTNode *body, int line);
ASTNode *create_function_call(const char *name, ASTNode *arguments, int line);
ASTNode *create_return_node(ASTNode *value, int line);
ASTNode *create_binary_node(BinaryOperator op, ASTNode *left, ASTNode *right, int line);

const char *data_type_name(DataType type);
const char *value_kind_name(ValueKind kind);
const char *binary_operator_name(BinaryOperator op);
void print_ast_table(const ASTNode *root);
void free_parameters(FunctionParameter *parameter);
void free_ast(ASTNode *node);

#endif
