#pragma once
#include <stdint.h>

typedef uint64_t entity_t;

#define ENTITY_INVALID UINT64_MAX

entity_t entity_create();

void entity_destroy(entity_t);