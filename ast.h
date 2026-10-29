#ifndef AST_H
#define AST_H

typedef enum {
  TYPE_RECURSO,
  TYPE_EPOCA,
  TYPE_MORAL,
  TYPE_HEROE,
  TYPE_ALIADO,
  TYPE_RECURSO_ARRAY,
  TYPE_EPOCA_ARRAY,
  TYPE_MORAL_ARRAY,
  TYPE_HEROE_ARRAY,
  TYPE_ALIADO_ARRAY,
  TYPE_NADA,
  TYPE_UNKNOWN
} DataType;

typedef enum {
  VALUE_INT,
  VALUE_FLOAT,
  VALUE_TEXT,
  VALUE_BOOL,
  VALUE_ARRAY,
  VALUE_NONE
} ValueKind;

typedef enum {
  OP_ADD,
  OP_SUB,
  OP_MUL,
  OP_DIV,
  OP_EQ,
  OP_NE,
  OP_GT,
  OP_LT,
  OP_GE,
  OP_LE,
} BinaryOperator;

typedef enum {
  VAR_DECL,
  ASSIGNMENT,
  FUNCTION_DECL,
  RETURN_STMT,
  WORLD_DECL,
  EMPIRE_DECL,
  SIZE_STMT,
  TERRAIN_STMT,
  WATER_STMT,
  POPULATE_STMT,
  TREASURE_STMT,
  IF_STMT,

  INT_LITERAL,
  FLOAT_LITERAL,
  STR_LITERAL,
  BOOL_LITERAL,
  IDENTIFIER_REF,
  FUNCTION_CALL,
  BINARY_OP,
  ARRAY_LITERAL,
  ARRAY_INDEX,
  ARRAY_METHOD_CALL,
} NodeType;

typedef enum {
  ARRAY_PUSH,
  ARRAY_POP,
  ARRAY_LENGTH,
  ARRAY_UNKNOWN_METHOD
} ArrayMethod;

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

typedef struct BlockDeclaration {
  char *identifier;
  struct ASTNode *body;
} BlockDeclaration;

typedef struct WorldItem {
  char *word;
  struct ASTNode *amount;
} WorldItem;

typedef struct ASTNode {
  NodeType type;
  int line;
  ValueKind valueKind;
  union {
    VarDeclaration varDeclaration;
    Assignment assignment;
    FunctionDeclaration functionDeclaration;
    BlockDeclaration block;
    WorldItem worldItem;
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
    struct {
      DataType elementType;
      struct ASTNode *elements;
    } arrayLiteral;
    struct {
      char *identifier;
      struct ASTNode *index;
    } arrayIndex;
    struct {
      char *identifier;
      char *methodName;
      ArrayMethod method;
      struct ASTNode *arguments;
    } arrayMethodCall;
    struct {
      struct ASTNode *condition;
      struct ASTNode *thenBody;
      struct ASTNode *elseBody;
    } ifStatement;
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
ASTNode *create_block_decl(NodeType type, const char *name, ASTNode *body, int line);
ASTNode *create_world_item(NodeType type, const char *word, ASTNode *amount, int line);
ASTNode *create_binary_node(BinaryOperator op, ASTNode *left, ASTNode *right, int line);
ASTNode *create_if_node(ASTNode *condition, ASTNode *then_body, ASTNode *else_body, int line);
ASTNode *create_array_node(ASTNode *elements, int line);
ASTNode *create_array_index(const char *name, ASTNode *index, int line);
ASTNode *create_array_method_call(const char *name, const char *method, ASTNode *arguments,
                                  int line);

DataType array_type_of(DataType element_type);
DataType array_element_type(DataType array_type);

const char *data_type_name(DataType type);
const char *value_kind_name(ValueKind kind);
const char *binary_operator_name(BinaryOperator op);
void print_ast_table(const ASTNode *root);
void free_parameters(FunctionParameter *parameter);
void free_ast(ASTNode *node);

#endif
