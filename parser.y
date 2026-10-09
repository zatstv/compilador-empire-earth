%{
#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int yylex();
extern int yylineno;
extern char *yytext;
extern int previous_token_line;
extern int previous_token;
extern int open_braces;
extern int parens_before_semicolon;
void yyerror(const char *s);

ASTNode *ast_root = NULL;
int syntax_errors = 0;

static void syntax_defeat(int line, const char *message, const char *name) {
    syntax_errors++;
    fprintf(stderr, "DERROTA en la linea %d: %s '%s'\n", line, message, name);
}
%}

%code requires {
#include "ast.h"
}

%union {
    int line;
    struct { long value; int line; } num;
    struct { double value; int line; } dec;
    struct { char *text; int line; } str;
    struct { int value; int line; } boolean;
    struct { DataType value; int line; } type;
    struct ASTNode *node;
    struct FunctionParameter *parameter;
    DataType data_type;
}

%token <line> TOKEN_PROGRAM_START TOKEN_PROGRAM_END TOKEN_RECRUIT TOKEN_ARROW
%token <line> TOKEN_FUNCTION TOKEN_RETURN TOKEN_NOTHING
%token <type> TOKEN_TYPE
%token <str> TOKEN_IDENTIFIER
%token <str> TOKEN_STRING_LITERAL
%token <num> TOKEN_INT_LITERAL
%token <dec> TOKEN_FLOAT_LITERAL
%token <boolean> TOKEN_BOOL_LITERAL

%destructor { free($$.text); } <str>
%destructor { free_ast($$); } <node>
%destructor { free_parameters($$); } <parameter>

%left '+' '-'
%left '*' '/'

//any of these non-terminal rules produce a result
%type <node> statement_list statement var_declaration identifier_list assignment expression
%type <node> function_declaration function_statement_list function_statement return_statement
%type <node> function_call optional_arguments argument_list
%type <parameter> optional_parameters parameter_list parameter
%type <data_type> return_type

%%

program:
    TOKEN_PROGRAM_START statement_list TOKEN_PROGRAM_END {
        ast_root = $2;
    }
    | TOKEN_PROGRAM_START TOKEN_PROGRAM_END {
        ast_root = NULL;
    }
    ;

statement_list:
    statement {
        $$ = $1;
    }
    | statement_list statement {
        /* Append to linked list */
        $$ = append_statement($1, $2);
    }
    ;

statement:
    var_declaration ';' {
        $$ = $1;
    }
    | assignment ';' {
        $$ = $1;
    }
    | function_declaration {
        $$ = $1;
    }
    | function_call ';' {
        $$ = $1;
    }
    | error ';' {
        yyerrok;
        $$ = NULL;
    }
    ;

function_declaration:
    TOKEN_FUNCTION TOKEN_IDENTIFIER '(' optional_parameters ')' ':' return_type '{' function_statement_list '}' {
        $$ = create_function_decl($7, $2.text, $4, $9, $2.line);
        free($2.text);
    }
    | TOKEN_FUNCTION error '}' {
        yyerrok;
        $$ = NULL;
    }
    ;

return_type:
    TOKEN_TYPE { $$ = $1.value; }
    | TOKEN_NOTHING { $$ = TYPE_NADA; }
    ;

optional_parameters:
    { $$ = NULL; }
    | parameter_list { $$ = $1; }
    ;

parameter_list:
    parameter { $$ = $1; }
    | parameter ',' parameter_list {
        $1->next = $3;
        $$ = $1;
    }
    ;

parameter:
    TOKEN_TYPE TOKEN_IDENTIFIER {
        $$ = create_function_parameter($1.value, $2.text, $2.line);
        free($2.text);
    }
    ;

function_statement_list:
    { $$ = NULL; }
    | function_statement_list function_statement {
        $$ = append_statement($1, $2);
    }
    ;

function_statement:
    var_declaration ';' { $$ = $1; }
    | assignment ';' { $$ = $1; }
    | return_statement { $$ = $1; }
    | function_call ';' { $$ = $1; }
    | error ';' {
        yyerrok;
        $$ = NULL;
    }
    ;

return_statement:
    TOKEN_RETURN expression ';' { $$ = create_return_node($2, $1); }
    | TOKEN_RETURN ';' { $$ = create_return_node(NULL, $1); }
    ;

function_call:
    TOKEN_IDENTIFIER '(' optional_arguments ')' {
        $$ = create_function_call($1.text, $3, $1.line);
        free($1.text);
    }
    ;

optional_arguments:
    { $$ = NULL; }
    | argument_list { $$ = $1; }
    ;

argument_list:
    expression { $$ = $1; }
    | expression ',' argument_list {
        $1->next = $3;
        $$ = $1;
    }
    ;

var_declaration:
    TOKEN_RECRUIT TOKEN_TYPE identifier_list {
        for (ASTNode *decl = $3; decl != NULL; decl = decl->next) {
            decl->data.varDeclaration.dataType = $2.value;
        }
        $$ = $3;
    }
    | TOKEN_RECRUIT TOKEN_TYPE TOKEN_IDENTIFIER TOKEN_ARROW expression {
        $$ = create_var_decl($2.value, $3.text, $5, $3.line);
        free($3.text);
    }
    | TOKEN_RECRUIT identifier_list {
        syntax_defeat($2->line, "le falta el tipo a la unidad", $2->data.varDeclaration.identifier);
        free_ast($2);
        $$ = NULL;
    }
    | TOKEN_TYPE identifier_list {
        syntax_defeat($2->line, "falta la palabra reclutar antes de", $2->data.varDeclaration.identifier);
        free_ast($2);
        $$ = NULL;
    }
    ;

identifier_list:
    TOKEN_IDENTIFIER {
        $$ = create_var_decl(TYPE_UNKNOWN, $1.text, NULL, $1.line);
        free($1.text);
    }
    | identifier_list ',' TOKEN_IDENTIFIER {
        $$ = append_statement($1, create_var_decl(TYPE_UNKNOWN, $3.text, NULL, $3.line));
        free($3.text);
    }
    ;

assignment:
    TOKEN_IDENTIFIER TOKEN_ARROW expression {
        $$ = create_assignment($1.text, $3, $1.line);
        free($1.text);
    }
    | TOKEN_IDENTIFIER '=' expression {
        syntax_defeat($1.line, "para asignar se usa <- y no =, revisa", $1.text);
        free($1.text);
        free_ast($3);
        $$ = NULL;
    }
    ;

expression:
    TOKEN_INT_LITERAL {
        $$ = create_int_node($1.value, $1.line);
    }
    | TOKEN_FLOAT_LITERAL {
        $$ = create_float_node($1.value, $1.line);
    }
    | TOKEN_STRING_LITERAL {
        $$ = create_str_node($1.text, $1.line);
        free($1.text);
    }
    | TOKEN_BOOL_LITERAL {
        $$ = create_bool_node($1.value, $1.line);
    }
    | TOKEN_IDENTIFIER {
        $$ = create_identifier_node($1.text, $1.line);
        free($1.text);
    }
    | function_call { $$ = $1; }
    | '(' expression ')' { $$ = $2; }
    | expression '+' expression { $$ = create_binary_node(OP_ADD, $1, $3, $1->line); }
    | expression '-' expression { $$ = create_binary_node(OP_SUB, $1, $3, $1->line); }
    | expression '*' expression { $$ = create_binary_node(OP_MUL, $1, $3, $1->line); }
    | expression '/' expression { $$ = create_binary_node(OP_DIV, $1, $3, $1->line); }
    ;

%%

void yyerror(const char *s) {
    (void)s;
    syntax_errors++;
    if (yytext == NULL || yytext[0] == '\0') {
        fprintf(stderr, "DERROTA en la linea %d: el programa se acabo sin la palabra victoria\n",
                yylineno);
    } else if (strcmp(yytext, "victoria") == 0 && open_braces > 0) {
        fprintf(stderr, "DERROTA en la linea %d: falta cerrar la estrategia con }\n", yylineno);
    } else if (strcmp(yytext, "estrategia") == 0 && open_braces > 0) {
        fprintf(stderr, "DERROTA en la linea %d: no se puede crear una estrategia dentro de otra\n",
                yylineno);
    } else if (strcmp(yytext, "tributo") == 0 && open_braces == 0) {
        fprintf(stderr, "DERROTA en la linea %d: tributo solo se usa dentro de una estrategia\n",
                yylineno);
    } else if ((yychar == TOKEN_TYPE || yychar == TOKEN_NOTHING) && previous_token == ')') {
        fprintf(stderr, "DERROTA en la linea %d: falta ':' antes del tipo de la estrategia\n",
                yylineno);
    } else if (yychar == ';' && parens_before_semicolon > 0) {
        fprintf(stderr, "DERROTA en la linea %d: falta cerrar el parentesis )\n", yylineno);
    } else if (previous_token_line == 0) {
        fprintf(stderr, "DERROTA en la linea %d: el programa tiene que empezar con fundar_ciudad\n",
                yylineno);
    } else if (previous_token_line < yylineno) {
        fprintf(stderr, "DERROTA en la linea %d: falta un ; al final de la linea\n",
                previous_token_line);
    } else if (strcmp(yytext, "victoria") == 0 || strcmp(yytext, "fundar_ciudad") == 0 ||
               strcmp(yytext, "reclutar") == 0 || strcmp(yytext, "paz") == 0 ||
               strcmp(yytext, "guerra") == 0 || strcmp(yytext, "estrategia") == 0 ||
               strcmp(yytext, "tributo") == 0 || strcmp(yytext, "nada") == 0) {
        fprintf(stderr, "DERROTA en la linea %d: '%s' es palabra reservada y no se puede usar ahi\n",
                yylineno, yytext);
    } else {
        fprintf(stderr, "DERROTA en la linea %d: error de sintaxis cerca de '%s'\n", yylineno, yytext);
    }
}
