#ifndef ESE_SYSTEM_TYPE
	#error "ESE_SYSTEM_TYPE must be defined before including system_generator.h"
#endif
ARRAY_DEFINITION(ESE_SYSTEM_TYPE)
ARRAY_IMPLEMENTATION(ESE_SYSTEM_TYPE)

#define ESE_HELPER(prepend, symbol) prepend ## _ ## symbol
#define ESE_JOIN_(prepend, symbol) ESE_HELPER(prepend, symbol)
#define ESE_JOIN(symbol) ESE_JOIN_(ESE_SYSTEM_TYPE, symbol)

typedef void(*ticker)(entity,ESE_SYSTEM_TYPE *);
static pthread_mutex_t active_mutex;
static pthread_mutex_t pending_mutex;
static pthread_mutex_t deletion_mutex;


ARRAY_VARIABLE(entity, entities);
ARRAY_VARIABLE(entity, removals);
ARRAY_VARIABLE(ESE_SYSTEM_TYPE, components);

void system_init_internal()
{
	pthread_mutex_init(&active_mutex, NULL);
	pthread_mutex_init(&pending_mutex, NULL);
	pthread_mutex_init(&deletion_mutex, NULL);

	#ifdef ESE_INIT_HOOK
	ESE_INIT_HOOK();
	#endif

}

typedef struct
{
	entity entity;
	ESE_SYSTEM_TYPE component;
} pending_component;

ARRAY_DEFINITION(pending_component)
ARRAY_VARIABLE(pending_component, pending_components);
ARRAY_IMPLEMENTATION(pending_component);

void add(entity e, void * component)
{
	pending_component pending = {e, *((ESE_SYSTEM_TYPE*) component)};
	pthread_mutex_lock(&pending_mutex);
	pending_component_array_append(&pending_components, &pending);
	pthread_mutex_unlock(&pending_mutex);
}


static void add_internal()
{
	
	pthread_mutex_lock(&active_mutex);
	pthread_mutex_lock(&pending_mutex);
	entity_array_reserve(&entities, pending_components.count);
	ARRAY_ITERATE(pending_components,
		entity_array_append(&entities, &pending_components.values[index].entity);
		ESE_JOIN(array_append)(&components, &pending_components.values[index].component)
		#ifdef ESE_ADD_HOOK
		ESE_ADD_HOOK(pending_components->values[index].entity, components.values + components.count - 1);
		#endif
	)
	pending_components.count = 0;
	pthread_mutex_unlock(&pending_mutex);
	pthread_mutex_unlock(&active_mutex);
}


static size_t find_idx(entity e)
{
	size_t idx = SIZE_MAX;
	pthread_mutex_lock(&active_mutex);
	ARRAY_ITERATE(entities,
		if (entities.values[index] != e);
		else
		{
			idx = index;
			break;
		}
	)
	pthread_mutex_unlock(&active_mutex);
	return idx;
}


void * find(entity entity)
{
	size_t idx = find_idx(entity);
	if (idx != SIZE_MAX)
	{
		return (components.values + idx);
	}
	return NULL;
}


void delete(entity entity)
{
	pthread_mutex_lock(&deletion_mutex);
	entity_array_append(&removals, &entity);
	pthread_mutex_unlock(&deletion_mutex);
}


static void delete_internal()
{
	pthread_mutex_lock(&active_mutex);
	pthread_mutex_lock(&deletion_mutex);
	for(size_t i = 0; i < entities.count; ++i)
	{
		entity e = entities.values[i];
		for( size_t j = 0; j < removals.count; ++j)
		{
			entity checker = removals.values[j];
			if (e != checker) {}\
			else
			{	
				#ifdef ESE_DELETE_HOOK
				ESE_DELETE_HOOK(entity, i);
				#endif
				ESE_JOIN(array_remove)(&components, i);
				entity_array_remove(&entities, i);
				--i;
				break;
			}
		}
	}
	removals.count = 0;
	pthread_mutex_unlock(&deletion_mutex);
	pthread_mutex_unlock(&active_mutex);
}


void tick_internal(uint64_t tick, uint16_t thread_id, uint64_t thread_count)
{
	#ifdef ESE_SYSTEM_TICK
		size_t count = (entities.count / thread_count) + 1;
		size_t start = count * thread_id;
		size_t end = start + count;
		for (size_t i = start; i < entities.count && i < end; ++i)
			ESE_SYSTEM_TICK(entities.values[i], (components.values + i));
	#endif
}


void resolve_internal()
{
	#ifdef ESE_RESOLVE_HOOK
	ESE_RESOLVE_HOOK();
	#endif
	delete_internal();
	add_internal();
}


system_functions functions = {add, find, delete, tick_internal, resolve_internal};