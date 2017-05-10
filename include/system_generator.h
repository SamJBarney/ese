#ifndef ESE_SYSTEM_TYPE
	#error "ESE_SYSTEM_TYPE must be defined before including system_generator.h"
#endif
ARRAY_DEFINITION(ESE_SYSTEM_TYPE)
ARRAY_IMPLEMENTATION(ESE_SYSTEM_TYPE)

#define ESE_HELPER(prepend, symbol) prepend ## _ ## symbol
#define ESE_JOIN_(prepend, symbol) ESE_HELPER(prepend, symbol)
#define ESE_JOIN(symbol) ESE_JOIN_(ESE_SYSTEM_TYPE, symbol)

typedef void(*ticker)(entity,ESE_SYSTEM_TYPE *);
static pthread_mutex_t ESE_JOIN(active_mutex);
static pthread_mutex_t ESE_JOIN(pending_mutex);
static pthread_mutex_t ESE_JOIN(deletion_mutex);


ARRAY_VARIABLE(entity, ESE_JOIN(entities));
ARRAY_VARIABLE(entity, ESE_JOIN(removals));
ARRAY_VARIABLE(ESE_SYSTEM_TYPE, components);

void ESE_JOIN(init)()
{
	pthread_mutex_init(&ESE_JOIN(active_mutex), NULL);
	pthread_mutex_init(&ESE_JOIN(pending_mutex), NULL);
	pthread_mutex_init(&ESE_JOIN(deletion_mutex), NULL);

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
ARRAY_VARIABLE(pending_component, ESE_JOIN(pending_components));
ARRAY_IMPLEMENTATION(pending_component);

void ESE_JOIN(add)(entity e, void * component)
{
	pending_component pending = {e, *((ESE_SYSTEM_TYPE*) component)};
	pthread_mutex_lock(&ESE_JOIN(pending_mutex));
	pending_component_array_append(&ESE_JOIN(pending_components), &pending);
	pthread_mutex_unlock(&ESE_JOIN(pending_mutex));
}


static void ESE_JOIN(add_internal)()
{
	
	pthread_mutex_lock(&ESE_JOIN(active_mutex));
	pthread_mutex_lock(&ESE_JOIN(pending_mutex));
	entity_array_reserve(&ESE_JOIN(entities), ESE_JOIN(pending_components).count);
	ARRAY_ITERATE(ESE_JOIN(pending_components),
		entity_array_append(&ESE_JOIN(entities), &ESE_JOIN(pending_components).values[index].entity);
		ESE_JOIN(array_append)(&components, &ESE_JOIN(pending_components).values[index].component)
		#ifdef ESE_ADD_HOOK
		ESE_ADD_HOOK(ESE_JOIN(pending_components)->values[index].entity, components.values + components.count - 1);
		#endif
	)
	ESE_JOIN(pending_components).count = 0;
	pthread_mutex_unlock(&ESE_JOIN(pending_mutex));
	pthread_mutex_unlock(&ESE_JOIN(active_mutex));
}


static size_t ESE_JOIN(find_idx)(entity e)
{
	size_t idx = SIZE_MAX;
	pthread_mutex_lock(&ESE_JOIN(active_mutex));
	ARRAY_ITERATE(ESE_JOIN(entities),
		if (ESE_JOIN(entities).values[index] != e);
		else
		{
			idx = index;
			break;
		}
	)
	pthread_mutex_unlock(&ESE_JOIN(active_mutex));
	return idx;
}


void * ESE_JOIN(find)(entity entity)
{
	size_t idx = ESE_JOIN(find_idx)(entity);
	if (idx != SIZE_MAX)
	{
		return (components.values + idx);
	}
	return NULL;
}


void ESE_JOIN(delete)(entity entity)
{
	pthread_mutex_lock(&ESE_JOIN(deletion_mutex));
	entity_array_append(&ESE_JOIN(removals), &entity);
	pthread_mutex_unlock(&ESE_JOIN(deletion_mutex));
}


static void ESE_JOIN(delete_internal)()
{
	pthread_mutex_lock(&ESE_JOIN(active_mutex));
	pthread_mutex_lock(&ESE_JOIN(deletion_mutex));
	for(size_t i = 0; i < ESE_JOIN(entities).count; ++i)
	{
		entity e = ESE_JOIN(entities).values[i];
		for( size_t j = 0; j < ESE_JOIN(removals).count; ++j)
		{
			entity checker = ESE_JOIN(removals).values[j];
			if (e != checker) {}\
			else
			{	
				#ifdef ESE_DELETE_HOOK
				ESE_DELETE_HOOK(entity, i);
				#endif
				ESE_JOIN(array_remove)(&components, i);
				entity_array_remove(&ESE_JOIN(entities), i);
				--i;
				break;
			}
		}
	}
	ESE_JOIN(removals).count = 0;
	pthread_mutex_unlock(&ESE_JOIN(deletion_mutex));
	pthread_mutex_unlock(&ESE_JOIN(active_mutex));
}


void ESE_JOIN(tick_internal)(uint64_t tick, uint16_t thread_id, uint64_t thread_count)
{
	#ifdef ESE_SYSTEM_TICK
		size_t count = (ESE_JOIN(entities).count / thread_count) + 1;
		size_t start = count * thread_id;
		size_t end = start + count;
		for (size_t i = start; i < ESE_JOIN(entities).count && i < end; ++i)
			ESE_SYSTEM_TICK(ESE_JOIN(entities).values[i], (components.values + i));
	#endif
}


static void ESE_JOIN(resolve_internal)()
{
	#ifdef ESE_RESOLVE_HOOK
	ESE_RESOLVE_HOOK();
	#endif
	ESE_JOIN(delete_internal)();
	ESE_JOIN(add_internal)();
}


system_t ESE_JOIN(system) = {ESE_JOIN(init), ESE_JOIN(add), ESE_JOIN(find), ESE_JOIN(delete), ESE_JOIN(tick_internal), ESE_JOIN(resolve_internal)};