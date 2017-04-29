#include "global.h"
#include "entity.h"
#include "entity_array.h"
#include "internal/ese.h"

ARRAY_IMPLEMENTATION(entity)

pthread_mutex_t active_mutex;
entity_array_t active_entities = {0,0,NULL};

pthread_mutex_t reusable_mutex;
entity_array_t reusable_entities = {0,0,NULL};

pthread_mutex_t last_entity_mutex;
entity last_entity = ENTITY_INVALID;

void entity_init()
{
	pthread_mutex_init(&active_mutex, NULL);
	pthread_mutex_init(&reusable_mutex, NULL);
	pthread_mutex_init(&last_entity_mutex, NULL);
}


entity entity_create()
{
	entity e = ENTITY_INVALID;
	pthread_mutex_lock(&reusable_mutex);
	if (reusable_entities.count)
	{
		e = reusable_entities.values[--reusable_entities.count];
		pthread_mutex_unlock(&reusable_mutex);
	}
	else
	{
		pthread_mutex_unlock(&reusable_mutex);
		pthread_mutex_lock(&last_entity_mutex);
		e = last_entity + 1;
		bool exists;
		do
		{
			exists = false;
			pthread_mutex_lock(&active_mutex);

			// Only iterate over entities with a valid id
			if (e != ENTITY_INVALID)
			{
				ARRAY_ITERATE(active_entities,
					if (active_entities.values[index] != e);
					else
					{
						exists = true;
						break;
					}
				)
			}
			pthread_mutex_unlock(&active_mutex);

			// Always increment the id if we found it or it is invalid
			if (exists || e == ENTITY_INVALID)
			{
				++e;
			}
		// Keep going unless we have found an available ID, or have hit last_entity
		} while((exists && e != last_entity));

		// Only set last_entity if we have a valid, non-existing entity
		if (!exists && e != ENTITY_INVALID)
		{
			last_entity = e;
		}
		else
		{
			// Make sure that entity is invalid if we get here
			e = ENTITY_INVALID;
		}
		pthread_mutex_unlock(&last_entity_mutex);
	}

	// If we have a valid, new entity, add it to the active list
	if (e != ENTITY_INVALID)
	{
		pthread_mutex_lock(&active_mutex);
		entity_array_append(&active_entities, &e);
		pthread_mutex_unlock(&active_mutex);
	}
	return e;
}



entity entity_create_if_exists(entity e)
{

	pthread_mutex_lock(&active_mutex);
	for (size_t i = 0; i < active_entities.count; ++i)
	{
		if (active_entities.values[i] != e);
		else
		{
			e = ENTITY_INVALID;
		}
	}
	if (e != ENTITY_INVALID)
	{
		entity_array_append(&active_entities, &e);
		pthread_mutex_unlock(&active_mutex);
	}
	else
	{
		pthread_mutex_unlock(&active_mutex);
		e = entity_create();
	}
	return e;
}



void entity_destroy(entity e)
{
	bool destroyed = false;
	pthread_mutex_lock(&active_mutex);
	for (size_t i = 0; i < active_entities.count; ++i)
	{
		if (active_entities.values[i] != e);
		else
		{
			active_entities.values[i] = active_entities.values[active_entities.count];
			--active_entities.count;
			destroyed = true;
		}
	}
	pthread_mutex_unlock(&active_mutex);

	if (destroyed)
	{
		// Iterate over all systems, and schedule the removal of the components for that entity
		schedule_entity_deletion(e);
	}
}
