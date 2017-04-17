#pragma once
#include "global.h"
#include "system.h"
#include "entity.h"

struct entity_array
{
	size_t size;
	size_t count;
	entity_t * entities;
};

#define create_system(type, tick_func, caching) \
\
\
typedef void(*ticker)(entity_t,type *); \
static pthread_mutex_t type ## _active_mutex; \
static pthread_mutex_t type ## _pending_mutex; \
static pthread_mutex_t type ## _deletion_mutex; \
\
\
void component_init_ ## type() \
{ \
	pthread_mutex_init(&type ## _active_mutex, NULL); \
	pthread_mutex_init(&type ## _pending_mutex, NULL); \
	pthread_mutex_init(&type ## _deletion_mutex, NULL); \
} \
\
\
static struct entity_array type ## _entities = {0,0,NULL}; \
static type * type ## _components = NULL; \
\
\
typedef struct \
{ \
	entity_t entity; \
	type component; \
} pending_component; \
\
\
static struct pending_components \
{ \
	size_t size; \
	size_t count; \
	pending_component * pending; \
} type ## _pending_components = {0,0,NULL}; \
\
\
static struct entity_array type ## _removals = {0,0,NULL}; \
\
\
void type ## _add(entity_t entity, void * component) \
{ \
	if (type ## _pending_components.size > type ## _pending_components.count) \
	{ \
		pthread_mutex_lock(&type ## _pending_mutex); \
		type ## _pending_components.pending[type ## _pending_components.count].component = *(type*)component; \
		type ## _pending_components.pending[type ## _pending_components.count].entity = entity; \
		++type ## _pending_components.count; \
		pthread_mutex_unlock(&type ## _pending_mutex); \
	} \
	else \
	{ \
		pthread_mutex_lock(&type ## _pending_mutex); \
		type ## _pending_components.size += (type ## _pending_components.size / 5) + 10; \
		type ## _pending_components.pending = realloc(type ## _pending_components.pending, type ## _pending_components.size * sizeof(pending_component)); \
		type ## _pending_components.pending[type ## _pending_components.count].component = *(type*)component; \
		type ## _pending_components.pending[type ## _pending_components.count].entity = entity; \
		++type ## _pending_components.count; \
		pthread_mutex_unlock(&type ## _pending_mutex); \
	} \
} \
\
\
static void type ## _add_internal() \
{ \
	if (type ## _entities.size > (type ## _entities.count + type ## _pending_components.count)) \
	{ \
		pthread_mutex_lock(&type ## _active_mutex); \
		pthread_mutex_lock(&type ## _pending_mutex); \
		for (size_t i = 0; i < type ## _pending_components.count; ++i) \
		{ \
			type ## _entities.entities[type ## _entities.count] = type ## _pending_components.pending[i].entity; \
			type ## _components[type ## _entities.count] = type ## _pending_components.pending[i].component; \
			++type ## _entities.count; \
		} \
		type ## _pending_components.count = 0; \
		pthread_mutex_unlock(&type ## _pending_mutex); \
		pthread_mutex_unlock(&type ## _active_mutex); \
	} \
	else \
	{ \
		pthread_mutex_lock(&type ## _active_mutex); \
		pthread_mutex_lock(&type ## _pending_mutex); \
		type ## _entities.size += (type ## _entities.size / 5) + type ## _pending_components.count; \
		type ## _entities.entities = realloc(type ## _entities.entities, type ## _entities.size * sizeof(entity_t)); \
		type ## _components = realloc(type ## _components, type ## _entities.size * sizeof(type)); \
		for (size_t i = 0; i < type ## _pending_components.count; ++i) \
		{ \
			type ## _entities.entities[type ## _entities.count] = type ## _pending_components.pending[i].entity; \
			type ## _components[type ## _entities.count] = type ## _pending_components.pending[i].component; \
			++type ## _entities.count; \
		} \
		type ## _pending_components.count = 0; \
		pthread_mutex_unlock(&type ## _pending_mutex); \
		pthread_mutex_unlock(&type ## _active_mutex); \
	} \
} \
\
\
static size_t type ## _find_idx(entity_t entity) \
{ \
	size_t idx = SIZE_MAX; \
	pthread_mutex_lock(&type ## _active_mutex); \
	for (size_t i = 0; i < type ## _entities.count; ++i) \
	{ \
		if (type ## _entities.entities[i] != entity); \
		else \
		{ \
			idx = i; \
			break; \
		} \
	} \
	pthread_mutex_unlock(&type ## _active_mutex); \
	return idx; \
} \
\
\
void * type ## _find(entity_t entity) \
{ \
	size_t idx = type ## _find_idx(entity); \
	if (idx != SIZE_MAX) \
	{ \
		return (type ## _components + idx); \
	} \
	return NULL; \
} \
\
\
void type ## _remove(entity_t entity) \
{ \
	if (type ## _removals.size > type ## _removals.count) \
	{ \
		pthread_mutex_lock(&type ## _deletion_mutex); \
		type ## _removals.entities[type ## _removals.count] = entity; \
		++type ## _removals.count; \
		pthread_mutex_unlock(&type ## _deletion_mutex); \
	} \
	else \
	{ \
		pthread_mutex_lock(&type ## _deletion_mutex); \
		type ## _removals.size += (type ## _removals.size / 5) + 10; \
		type ## _removals.entities = realloc(type ## _removals.entities, type ## _removals.size * sizeof(type)); \
		type ## _removals.entities[type ## _removals.count] = entity; \
		++type ## _removals.count; \
		pthread_mutex_unlock(&type ## _deletion_mutex); \
	} \
} \
\
\
static void type ## _remove_internal() \
{ \
	pthread_mutex_lock(&type ## _active_mutex); \
	pthread_mutex_lock(&type ## _deletion_mutex); \
	size_t count = type ## _entities.count; \
	for(size_t i = 0; i <count; ++i) \
	{ \
		entity_t entity = type ## _entities.entities[i]; \
		for( size_t j = 0; j < type ## _removals.count; ++j) \
		{ \
			entity_t checker = type ## _removals.entities[j]; \
			if (entity != checker) {}\
			else \
			{ \
				type ## _components[i] = type ## _components[type ## _entities.count]; \
				type ## _entities.entities[i] = type ## _entities.entities[type ## _entities.count]; \
				--type ## _entities.count; \
				break; \
			} \
		} \
	} \
	type ## _removals.count = 0; \
	pthread_mutex_unlock(&type ## _deletion_mutex); \
	pthread_mutex_unlock(&type ## _active_mutex); \
} \
\
\
void type ## _save(FILE * file) \
{ \
	if (caching) \
	{ \
		pthread_mutex_lock(&type ## _active_mutex); \
		fwrite(&(type ## _entities.count), sizeof(size_t), 1, file); \
		fwrite(type ## _entities.entities, sizeof(entity_t), type ## _entities.count, file); \
		fwrite(type ## _components, sizeof(type), type ## _entities.count, file); \
		pthread_mutex_unlock(&type ## _active_mutex); \
		pthread_mutex_lock(&type ## _pending_mutex); \
		fwrite(&(type ## _pending_components.count), sizeof(size_t), 1, file); \
		fwrite(type ## _pending_components.pending, sizeof(pending_component), type ## _pending_components.count, file); \
		pthread_mutex_unlock(&type ## _pending_mutex); \
		pthread_mutex_lock(&type ## _deletion_mutex); \
		fwrite(&(type ## _removals.count), sizeof(size_t), 1, file); \
		fwrite(type ## _removals.entities, sizeof(entity_t), type ## _entities.count, file); \
		pthread_mutex_unlock(&type ## _deletion_mutex); \
	} \
} \
\
\
void type ## _load(FILE * file) \
{ \
	if (caching) \
	{ \
		pthread_mutex_lock(&type ## _active_mutex); \
		if (fread(&(type ## _entities.count), sizeof(size_t), 1, file) == 1) \
		{ \
			fread(type ## _entities.entities, sizeof(entity_t), type ## _entities.count, file); \
			fread(type ## _components, sizeof(type), type ## _entities.count, file); \
		} \
		pthread_mutex_unlock(&type ## _active_mutex); \
		pthread_mutex_lock(&type ## _pending_mutex); \
		if (fread(&(type ## _pending_components.count), sizeof(size_t), 1, file) == 1) \
		{ \
			fread(type ## _pending_components.pending, sizeof(pending_component), type ## _pending_components.count, file); \
		} \
		pthread_mutex_unlock(&type ## _pending_mutex); \
		pthread_mutex_lock(&type ## _deletion_mutex); \
		if (fread(&(type ## _removals.count), sizeof(size_t), 1, file) == 1) \
		{ \
			fread(type ## _removals.entities, sizeof(entity_t), type ## _removals.count, file); \
		} \
		pthread_mutex_unlock(&type ## _deletion_mutex); \
	} \
} \
\
\
void type ## _tick(uint64_t tick, uint16_t thread_id, uint64_t thread_count) \
{ \
	if (((void*)tick_func) != NULL) \
	{ \
		size_t count = (type ## _entities.count / thread_count) + 1; \
		size_t start = count * thread_id; \
		size_t end = start + count; \
		for (size_t i = start; i < type ## _entities.count && i < end; ++i) \
			((ticker)tick_func)(type ## _entities.entities[i], (type ## _components + i)); \
	} \
} \
\
\
void type ## _resolve() \
{ \
	type ## _add_internal(); \
	type ## _remove_internal(); \
} \
\
\
system_functions type ## _functions = {type ## _add, type ## _find, type ## _remove, type ## _tick, type ## _save, type ## _load, type ## _resolve}; \
\
\
