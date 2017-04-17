#pragma once
#include <stdint.h>

typedef uint64_t entity_t;

entity_t entity_create();

void entity_destroy(entity_t);