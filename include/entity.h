#pragma once
#include <stdint.h>

typedef uint64_t entity;

#define ENTITY_INVALID UINT64_MAX

entity entity_create();

entity entitity_create_if_exists(entity);

void entity_destroy(entity);