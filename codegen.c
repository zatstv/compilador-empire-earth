#include "codegen.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define MAX_UNITS_PER_KIND 50
#define MAX_TREASURE_PER_KIND 20
#define MAX_KINDS 16

typedef struct {
  const char *word;
  const char *game;
} Word;

static const Word units[] = {
    {"ciudadano", "Citizen"},          {"ciudadana", "FemaleCitizen"},
    {"garrotero", "Inf01-Clubman"},    {"mosquetero", "Inf08-Musketeer"},
    {"elefante", "Cav04-WarElephant"}, {"panzer", "Tank11Panzer"},
    {"tanque", "Tank12M1A1"},          {"zeus", "MechZeus"},
    {NULL, NULL},
};

static const Word resources[] = {
    {"oro", "Gold"}, {"hierro", "Iron"}, {"piedra", "Stone"}, {"bayas", "Berry"}, {NULL, NULL},
};

static const Word maps[] = {
    {"pradera", "Plains"},
    {"montanas", "Highlands"},
    {"continente", "Continental"},
    {"islas", "Large Islands"},
    {"islitas", "Small Islands"},
    {"mediterraneo", "Mediterranean"},
    {"torneo", "Tournament Islands"},
    {NULL, NULL},
};

static const Word sizes[] = {
    {"diminuto", "Diminuto (Tiny)"}, {"pequeno", "Pequeno (Small)"},
    {"mediano", "Mediano (Medium)"}, {"grande", "Grande (Large)"},
    {"enorme", "Enorme (Huge)"},     {"gigante", "Gigante (Gigantic)"},
    {NULL, NULL},
};

static const char *find(const Word *list, const char *word) {
  for (; list->word != NULL; list++) {
    if (strcmp(list->word, word) == 0) return list->game;
  }
  return NULL;
}

const char *game_unit_name(const char *word) { return find(units, word); }
const char *game_resource_name(const char *word) { return find(resources, word); }
const char *game_map_name(const char *word) { return find(maps, word); }
const char *game_size_name(const char *word) { return find(sizes, word); }

static const char *join(const Word *list, char *buffer, size_t size) {
  size_t used = 0;
  buffer[0] = '\0';
  for (; list->word != NULL && used < size; list++) {
    used += snprintf(buffer + used, size - used, "%s%s", used ? ", " : "", list->word);
  }
  return buffer;
}

const char *valid_words(const char *setting) {
  static char buffer[256];
  if (strcmp(setting, "poblar") == 0) return join(units, buffer, sizeof buffer);
  if (strcmp(setting, "tesoro") == 0) return join(resources, buffer, sizeof buffer);
  if (strcmp(setting, "terreno") == 0) return join(maps, buffer, sizeof buffer);
  return join(sizes, buffer, sizeof buffer);
}

int program_has_world(const ASTNode *program) {
  for (; program != NULL; program = program->next) {
    if (program->type == WORLD_DECL) return 1;
  }
  return 0;
}

typedef struct {
  const char *word;
  long amount;
  int line;
} Kind;

typedef struct {
  const char *name;
  const char *size;
  const char *terrain;
  long water;
  int hasWater;
  Kind units[MAX_KINDS];
  int unitCount;
  Kind treasures[MAX_KINDS];
  int treasureCount;
  int merged;
} Plan;

static int defeat(int line, const char *message) {
  fprintf(stderr, "DERROTA en la linea %d: %s\n", line, message);
  return 0;
}

static const Symbol *value_of(const SymbolTable *table, const ASTNode *item) {
  for (const Symbol *symbol = table->head; symbol != NULL; symbol = symbol->next) {
    if (symbol->item == item && symbol->hasValue) return symbol;
  }
  return NULL;
}

static int add_kind(Plan *plan, Kind *list, int *count, const ASTNode *item, long amount) {
  for (int i = 0; i < *count; i++) {
    if (strcmp(list[i].word, item->data.worldItem.word) == 0) {
      list[i].amount += amount;
      plan->merged++;
      return 1;
    }
  }
  if (*count == MAX_KINDS) return defeat(item->line, "hay demasiadas cosas distintas en el imperio");
  list[*count].word = item->data.worldItem.word;
  list[*count].amount = amount;
  list[*count].line = item->line;
  (*count)++;
  return 1;
}

static int build_plan(const ASTNode *program, const SymbolTable *table, Plan *plan) {
  memset(plan, 0, sizeof *plan);
  plan->size = "mediano";
  plan->terrain = "pradera";
  for (const ASTNode *node = program; node != NULL; node = node->next) {
    if (node->type != WORLD_DECL && node->type != EMPIRE_DECL) continue;
    if (node->type == WORLD_DECL) plan->name = node->data.block.identifier;
    for (const ASTNode *item = node->data.block.body; item != NULL; item = item->next) {
      const Symbol *symbol = value_of(table, item);
      if (symbol == NULL) return defeat(item->line, "no se pudo calcular este valor");
      switch (item->type) {
      case SIZE_STMT: plan->size = symbol->value.strValue; break;
      case TERRAIN_STMT: plan->terrain = symbol->value.strValue; break;
      case WATER_STMT:
        if (symbol->value.intValue > 10) {
          return defeat(item->line, "el agua va de 0 (nada) a 10 (casi todo agua)");
        }
        plan->water = symbol->value.intValue;
        plan->hasWater = 1;
        break;
      case POPULATE_STMT:
        if (!add_kind(plan, plan->units, &plan->unitCount, item, symbol->value.intValue)) return 0;
        break;
      case TREASURE_STMT:
        if (!add_kind(plan, plan->treasures, &plan->treasureCount, item, symbol->value.intValue)) {
          return 0;
        }
        break;
      default: break;
      }
    }
  }
  for (int i = 0; i < plan->unitCount; i++) {
    if (plan->units[i].amount > MAX_UNITS_PER_KIND) {
      fprintf(stderr, "DERROTA en la linea %d: no caben mas de %d %s por jugador (pediste %ld)\n",
              plan->units[i].line, MAX_UNITS_PER_KIND, plan->units[i].word, plan->units[i].amount);
      return 0;
    }
  }
  for (int i = 0; i < plan->treasureCount; i++) {
    if (plan->treasures[i].amount > MAX_TREASURE_PER_KIND) {
      fprintf(stderr, "DERROTA en la linea %d: no caben mas de %d minas de %s por jugador (pediste %ld)\n",
              plan->treasures[i].line, MAX_TREASURE_PER_KIND, plan->treasures[i].word,
              plan->treasures[i].amount);
      return 0;
    }
  }
  return 1;
}

static const char *maps_folder(void) {
  static char path[1024];
  const char *custom = getenv("EE_MAPAS");
  if (custom != NULL && custom[0] != '\0') return custom;
  const char *home = getenv("HOME");
  snprintf(path, sizeof path,
           "%s/.wine/drive_c/Program Files (x86)/Empire Earth/Empire Earth/Data/Random Map Scripts",
           home ? home : ".");
  return path;
}

static int valid_file_name(const char *name) {
  if (name[0] == '\0' || name[0] == ' ') return 0;
  for (const char *c = name; *c; c++) {
    if (!isalnum((unsigned char)*c) && *c != ' ' && *c != '-' && *c != '_') return 0;
  }
  for (const Word *map = maps; map->word != NULL; map++) {
    if (strcasecmp(map->game, name) == 0) return 0;
  }
  return 1;
}

static int write_map(const Plan *plan, const char *source_path, const char *folder, char *out,
                     size_t out_size) {
  char base_path[1200];
  snprintf(base_path, sizeof base_path, "%s/%s.rmv", folder, game_map_name(plan->terrain));
  FILE *base = fopen(base_path, "r");
  if (base == NULL) {
    fprintf(stderr, "DERROTA: no encontre el mapa base '%s'\n", base_path);
    fprintf(stderr, "         revisa que el juego este instalado o pon la carpeta en EE_MAPAS\n");
    return 0;
  }
  snprintf(out, out_size, "%s/%s.rmv", folder, plan->name);
  FILE *map = fopen(out, "w");
  if (map == NULL) {
    fclose(base);
    fprintf(stderr, "DERROTA: no se pudo crear '%s'\n", out);
    return 0;
  }

  char line[1024];
  while (fgets(line, sizeof line, base) != NULL) {
    if (strncmp(line, "StringTableID", 13) == 0) continue;
    if (plan->hasWater && strncmp(line, "PercentLand", 11) == 0) {
      fprintf(map, "PercentLand\t\t\t\t\t\t%.2f\r\n", 1.0 - plan->water / 10.0);
      continue;
    }
    fputs(line, map);
  }
  fclose(base);

  const char *file = strrchr(source_path, '/');
  fprintf(map, "\r\n// Generado por el compilador Empire Earth desde %s\r\n",
          file ? file + 1 : source_path);
  fprintf(map, "// Mundo \"%s\": terreno %s, tamano %s\r\n\r\n", plan->name, plan->terrain,
          plan->size);
  double x = -3;
  for (int i = 0; i < plan->unitCount; i++) {
    const char *game = game_unit_name(plan->units[i].word);
    double step = strstr(game, "Citizen") ? 0.5 : 1.0;
    for (long k = 0; k < plan->units[i].amount; k++) {
      if (k > 0 && k % 5 == 0) x -= step;
      fprintf(map, "PlaceUnit\t\t\t\t(%s,\t\t%.1f,\t%.1f,\t\t180)\r\n", game, x,
              -3 + (k % 5) * step);
    }
    if (plan->units[i].amount > 0) x -= step + 0.5;
  }
  fprintf(map, "\r\n");
  for (int i = 0; i < plan->treasureCount; i++) {
    const char *game = game_resource_name(plan->treasures[i].word);
    for (long k = 0; k < plan->treasures[i].amount; k++) {
      fprintf(map, "PlaceResourceInRing (%s, 6, 8)\r\n", game);
    }
  }
  fclose(map);
  return 1;
}

int generate_map(const ASTNode *program, const SymbolTable *table, const char *source_path) {
  Plan plan;
  if (!build_plan(program, table, &plan)) return 0;
  if (!valid_file_name(plan.name)) {
    fprintf(stderr, "DERROTA: el mundo \"%s\" necesita otro nombre (solo letras, numeros, espacios, "
                    "- y _, y que no sea el de un mapa del juego)\n", plan.name);
    return 0;
  }
  char out[1300];
  if (!write_map(&plan, source_path, maps_folder(), out, sizeof out)) return 0;

  printf("VICTORIA: el mundo \"%s\" se compilo sin errores.\n", plan.name);
  printf("  Mapa base:  %s (terreno %s)", game_map_name(plan.terrain), plan.terrain);
  if (plan.hasWater) printf(", agua %ld de 10", plan.water);
  printf("\n  Tamano:     escoger %s en el menu de la partida\n", game_size_name(plan.size));
  printf("  Cada jugador empieza con lo normal del mapa, y ademas:\n");
  if (plan.unitCount == 0 && plan.treasureCount == 0) printf("    (nada extra)\n");
  for (int i = 0; i < plan.unitCount; i++) {
    printf("    %ld %s\n", plan.units[i].amount, plan.units[i].word);
  }
  for (int i = 0; i < plan.treasureCount; i++) {
    printf("    %ld mina(s) de %s\n", plan.treasures[i].amount, plan.treasures[i].word);
  }
  if (plan.merged > 0) {
    printf("  Optimizacion: se juntaron %d instruccion(es) repetida(s)\n", plan.merged);
  }
  printf("  Archivo creado: %s\n", out);
  printf("  Para jugarlo: Empire Earth > Un jugador > Mapa aleatorio > tipo de mapa \"%s\"\n",
         plan.name);
  return 1;
}
