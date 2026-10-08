#include "ast.h"
#include "runtime.h"
#include "semantic.h"
#include "symbol_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern FILE *yyin;               // Flex input file pointer
extern int yyparse(void);        // Bison parse entry point
extern ASTNode *ast_root;        // Root node populated by parser.y
extern int syntax_errors;
extern int lexical_errors;
extern int show_tokens;

typedef struct {
  const char *input_path;
} Options;

static int parse_arguments(int argc, char **argv, Options *options) {
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-t") == 0) {
      show_tokens = 1;
      continue;
    }
    if (argv[i][0] == '-' || options->input_path != NULL) {
      return 0;
    }
    options->input_path = argv[i];
  }
  return options->input_path != NULL;
}

static void print_usage(const char *program_name) {
  fprintf(stderr, "Uso: %s [-t] archivo.ee\n", program_name);
  fprintf(stderr, "  -t  muestra los tokens que encuentra el analizador lexico\n");
}

static void title(const char *text) { printf("\n=== %s ===\n", text); }

int main(int argc, char **argv) {
  Options options = {0};
  SymbolTable symbol_table;

  setvbuf(stdout, NULL, _IOLBF, 0);

  if (!parse_arguments(argc, argv, &options)) {
    print_usage(argv[0]);
    return EXIT_FAILURE;
  }
  yyin = fopen(options.input_path, "r");
  if (!yyin) {
    perror("Error opening the file");
    return EXIT_FAILURE;
  }

  printf("Compilador Empire Earth - archivo: %s\n", options.input_path);

  title(show_tokens ? "1. Analisis lexico (tokens) y sintactico" : "1. Analisis lexico y sintactico");
  int parse_result = yyparse();
  fclose(yyin);
  yyin = NULL;

  int first_errors = lexical_errors + syntax_errors;
  if (parse_result != 0 || first_errors > 0) {
    printf("El programa no compila: %d derrota(s) lexica(s) o de sintaxis.\n",
           first_errors > 0 ? first_errors : 1);
    free_ast(ast_root);
    return EXIT_FAILURE;
  }
  printf("Correcto: el programa esta bien escrito.\n");

  title("2. Arbol de sintaxis (AST)");
  print_ast_table(ast_root);

  title("3. Analisis semantico");
  symbol_table_init(&symbol_table);
  int semantic_errors = semantic_analysis(ast_root, &symbol_table);
  if (semantic_errors > 0) {
    printf("El programa no compila: %d derrota(s) semantica(s).\n", semantic_errors);
    title("Tabla de simbolos");
    symbol_table_print(&symbol_table);
    free_ast(ast_root);
    symbol_table_destroy(&symbol_table);
    return EXIT_FAILURE;
  }
  printf("Correcto: todas las unidades existen y los tipos cuadran.\n");

  title("4. Ejecucion");
  int execution_succeeded = execute_program(ast_root, &symbol_table);
  if (execution_succeeded) {
    printf("VICTORIA: el programa se ejecuto sin errores.\n");
  } else {
    printf("El programa se detuvo por una derrota.\n");
  }

  title("5. Tabla de simbolos");
  symbol_table_print(&symbol_table);

  /* Clean up the AST heap allocations */
  free_ast(ast_root);
  symbol_table_destroy(&symbol_table);
  return execution_succeeded ? EXIT_SUCCESS : EXIT_FAILURE;
}
