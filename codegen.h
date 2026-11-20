#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"
#include "symbol_table.h"

const char *game_unit_name(const char *word);
const char *game_resource_name(const char *word);
const char *game_map_name(const char *word);
const char *game_size_name(const char *word);
const char *valid_words(const char *setting);

int program_has_world(const ASTNode *program);
int generate_map(const ASTNode *program, const SymbolTable *table, const char *source_path);

#endif
