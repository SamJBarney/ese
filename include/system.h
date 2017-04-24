#pragma once
#include <stdio.h>
#include "global.h"
#include "entity.h"
#include "entity_array.h"

typedef void (*system_add)(entity_t,void*);
typedef void * (*system_find)(entity_t);
typedef void (*system_remove)(entity_t);
typedef void (*system_tick)(uint64_t, uint16_t, uint64_t);
typedef void (*system_resolve)();

typedef struct
{
	system_add add;
	system_find find;
	system_remove remove;
	system_tick tick;
	system_resolve resolve;
} system_functions;