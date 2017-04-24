


#ifndef ESE_SYSTEM_TYPE
	#error "ESE_SYSTEM_TYPE must be defined before including system_generator.h"
#endif

#ifndef ESE_JOIN
	#define ESE_HELPER(prepend, symbol) prepend ## _ ## symbol
	#define ESE_JOIN_(prepend, symbol) ESE_HELPER(prepend, symbol)
	#define ESE_JOIN(symbol) ESE_JOIN_(ESE_SYSTEM_TYPE, symbol)
#endif

typedef void(*ticker)(entity_t,ESE_SYSTEM_TYPE *);
static pthread_mutex_t ESE_JOIN(active_mutex);
static pthread_mutex_t ESE_JOIN(pending_mutex);
static pthread_mutex_t ESE_JOIN(deletion_mutex);


static entity_array_t ESE_JOIN(entities) = {0,0,NULL};
static entity_array_t ESE_JOIN(removals) = {0,0,NULL};

static ESE_SYSTEM_TYPE * ESE_JOIN(components) = NULL;

void ESE_JOIN(system_init_internal)()
{
	pthread_mutex_init(&ESE_JOIN(active_mutex), NULL);
	pthread_mutex_init(&ESE_JOIN(pending_mutex), NULL);
	pthread_mutex_init(&ESE_JOIN(deletion_mutex), NULL);

	#ifdef ESE_CUSTOM_INIT
	ESE_JOIN(system_init)();
	#endif

}

typedef struct
{
	entity_t entity;
	ESE_SYSTEM_TYPE component;
} ESE_JOIN(pending_component);

static struct
{
	size_t size;
	size_t count;
	ESE_JOIN(pending_component) * pending;
} ESE_JOIN(pending_components) = {0,0,NULL};

void ESE_JOIN(add)(entity_t entity, void * component)
{
	if (ESE_JOIN(pending_components).size > ESE_JOIN(pending_components).count)
	{
		pthread_mutex_lock(&ESE_JOIN(pending_mutex));
		ESE_JOIN(pending_components).pending[ESE_JOIN(pending_components).count].component = *(ESE_SYSTEM_TYPE *)component;
		ESE_JOIN(pending_components).pending[ESE_JOIN(pending_components).count].entity = entity;
		++ESE_JOIN(pending_components).count;
		pthread_mutex_unlock(&ESE_JOIN(pending_mutex));
	}
	else
	{
		pthread_mutex_lock(&ESE_JOIN(pending_mutex));
		ESE_JOIN(pending_components).size += (ESE_JOIN(pending_components).size / 5) + 10;
		ESE_JOIN(pending_components).pending = realloc(ESE_JOIN(pending_components).pending, ESE_JOIN(pending_components).size * sizeof(ESE_JOIN(pending_component)));
		ESE_JOIN(pending_components).pending[ESE_JOIN(pending_components).count].component = *(ESE_SYSTEM_TYPE *)component;
		ESE_JOIN(pending_components).pending[ESE_JOIN(pending_components).count].entity = entity;
		++ESE_JOIN(pending_components).count;
		pthread_mutex_unlock(&ESE_JOIN(pending_mutex));
	}
}


static void ESE_JOIN(add_internal)()
{
	if (ESE_JOIN(entities).size > (ESE_JOIN(entities).count + ESE_JOIN(pending_components).count))
	{
		pthread_mutex_lock(&ESE_JOIN(active_mutex));
		pthread_mutex_lock(&ESE_JOIN(pending_mutex));
		for (size_t i = 0; i < ESE_JOIN(pending_components).count; ++i)
		{
			ESE_JOIN(entities).entities[ESE_JOIN(entities).count] = ESE_JOIN(pending_components).pending[i].entity;
			ESE_JOIN(components)[ESE_JOIN(entities).count] = ESE_JOIN(pending_components).pending[i].component;
			++ESE_JOIN(entities).count;
		}
		ESE_JOIN(pending_components).count = 0;
		pthread_mutex_unlock(&ESE_JOIN(pending_mutex));
		pthread_mutex_unlock(&ESE_JOIN(active_mutex));
	}
	else
	{
		pthread_mutex_lock(&ESE_JOIN(active_mutex));
		pthread_mutex_lock(&ESE_JOIN(pending_mutex));
		ESE_JOIN(entities).size += (ESE_JOIN(entities).size / 5) + ESE_JOIN(pending_components).count;
		ESE_JOIN(entities).entities = realloc(ESE_JOIN(entities).entities, ESE_JOIN(entities).size * sizeof(entity_t));
		ESE_JOIN(components) = realloc(ESE_JOIN(components), ESE_JOIN(entities).size * sizeof(ESE_SYSTEM_TYPE));
		for (size_t i = 0; i < ESE_JOIN(pending_components).count; ++i)
		{
			ESE_JOIN(entities).entities[ESE_JOIN(entities).count] = ESE_JOIN(pending_components).pending[i].entity;
			ESE_JOIN(components)[ESE_JOIN(entities).count] = ESE_JOIN(pending_components).pending[i].component;
			++ESE_JOIN(entities).count;
		}
		ESE_JOIN(pending_components).count = 0;
		pthread_mutex_unlock(&ESE_JOIN(pending_mutex));
		pthread_mutex_unlock(&ESE_JOIN(active_mutex));
	}
}


static size_t ESE_JOIN(find_idx)(entity_t entity)
{
	size_t idx = SIZE_MAX;
	pthread_mutex_lock(&ESE_JOIN(active_mutex));
	for (size_t i = 0; i < ESE_JOIN(entities).count; ++i)
	{
		if (ESE_JOIN(entities).entities[i] != entity);
		else
		{
			idx = i;
			break;
		}
	}
	pthread_mutex_unlock(&ESE_JOIN(active_mutex));
	return idx;
}


void * ESE_JOIN(find)(entity_t entity)
{
	size_t idx = ESE_JOIN(find_idx)(entity);
	if (idx != SIZE_MAX)
	{
		return (ESE_JOIN(components) + idx);
	}
	return NULL;
}


void ESE_JOIN(remove)(entity_t entity)
{
	if (ESE_JOIN(removals).size > ESE_JOIN(removals).count)
	{
		pthread_mutex_lock(&ESE_JOIN(deletion_mutex));
		ESE_JOIN(removals).entities[ESE_JOIN(removals).count] = entity;
		++ESE_JOIN(removals).count;
		pthread_mutex_unlock(&ESE_JOIN(deletion_mutex));
	}
	else
	{
		pthread_mutex_lock(&ESE_JOIN(deletion_mutex));
		ESE_JOIN(removals).size += (ESE_JOIN(removals).size / 5) + 10;
		ESE_JOIN(removals).entities = realloc(ESE_JOIN(removals).entities, ESE_JOIN(removals).size * sizeof(ESE_SYSTEM_TYPE));
		ESE_JOIN(removals).entities[ESE_JOIN(removals).count] = entity;
		++ESE_JOIN(removals).count;
		pthread_mutex_unlock(&ESE_JOIN(deletion_mutex));
	}
}


static void ESE_JOIN(remove_internal)()
{
	pthread_mutex_lock(&ESE_JOIN(active_mutex));
	pthread_mutex_lock(&ESE_JOIN(deletion_mutex));
	size_t count = ESE_JOIN(entities).count;
	for(size_t i = 0; i <count; ++i)
	{
		entity_t entity = ESE_JOIN(entities).entities[i];
		for( size_t j = 0; j < ESE_JOIN(removals).count; ++j)
		{
			entity_t checker = ESE_JOIN(removals).entities[j];
			if (entity != checker) {}\
			else
			{
				ESE_JOIN(components)[i] = ESE_JOIN(components)[ESE_JOIN(entities).count];
				ESE_JOIN(entities).entities[i] = ESE_JOIN(entities).entities[ESE_JOIN(entities).count];
				--ESE_JOIN(entities).count;
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
		size_t count = (ESE_JOIN(entities).count / thread_count) + 1;
		size_t start = count * thread_id;
		size_t end = start + count;
		for (size_t i = start; i < ESE_JOIN(entities).count && i < end; ++i)
			ESE_JOIN(tick)(ESE_JOIN(entities).entities[i], (ESE_JOIN(components) + i));
}


void ESE_JOIN(resolve_internal)()
{
	ESE_JOIN(remove_internal)();
	ESE_JOIN(add_internal)();
	#ifdef ESE_CUSTOM_RESOLVER
	ESE_JOIN(resolve)();
	#endif
}


system_functions ESE_JOIN(functions) = {ESE_JOIN(add), ESE_JOIN(find), ESE_JOIN(remove), ESE_JOIN(tick_internal), ESE_JOIN(resolve_internal)};

#undef ESE_SYSTEM_TYPE
#ifdef ESE_CUSTOM_INIT
	#undef ESE_CUSTOM_INIT
#endif
#ifdef ESE_CUSTOM_RESOLVER
	#undef ESE_CUSTOM_RESOLVER
#endif