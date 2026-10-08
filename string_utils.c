#include "string_utils.h"

#include <stdlib.h>
#include <string.h>

char *string_copy(const char *value) {
  size_t length = strlen(value) + 1;
  char *copy = malloc(length);
  if (copy != NULL) {
    memcpy(copy, value, length);
  }
  return copy;
}
