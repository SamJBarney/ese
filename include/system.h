#pragma once
#include <stdio.h>
#include "global.h"
#include "entity.h"
#include "entity_array.h"

typedef void (*system_init)();
typedef void (*system_add)(entity,void*);
typedef void * (*system_find)(entity);
typedef void (*system_remove)(entity);
typedef void (*system_tick)(uint64_t, uint16_t, uint64_t);
typedef void (*system_resolve)();

typedef struct
{
	const system_init init;
	const system_add add;
	const system_find find;
	const system_remove remove;
	const system_tick tick;
	const system_resolve resolve;
} system_t;

#define SYSTEM_DEFINITION(type)\
void type ## _add(entity, void *);\
void * type ## _find(entity);\
void type ## _delete(entity)