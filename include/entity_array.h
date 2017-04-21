#pragma once
#include "entity.h"

typedef struct
{
	size_t size;
	size_t count;
	entity_t * entities;
} entity_array_t;