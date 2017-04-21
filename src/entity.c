#include "global.h"
#include "entity.h"
#include "entity_array.h"
#include "internal/ese.h"

pthread_mutex_t active_mutex;
entity_array_t active_entities = {0,0,NULL};

pthread_mutex_t reusable_mutex;
entity_array_t reusable_entities = {0,0,NULL};

pthread_mutex_t last_entity_mutex;
entity_t last_entity = ENTITY_INVALID;

void entity_init()
{
	pthread_mutex_init(&active_mutex, NULL);
	pthread_mutex_init(&reusable_mutex, NULL);
	pthread_mutex_init(&last_entity_mutex, NULL);
}


entity_t entity_create()
{
	entity_t entity = ENTITY_INVALID;
	pthread_mutex_lock(&reusable_mutex);
	if (reusable_entities.count)
	{
		entity = reusable_entities.entities[--reusable_entities.count];
		pthread_mutex_unlock(&reusable_mutex);
	}
	else
	{
		pthread_mutex_unlock(&reusable_mutex);
		pthread_mutex_lock(&last_entity_mutex);
		entity = last_entity + 1;
		bool exists;
		do
		{
			exists = false;
			pthread_mutex_lock(&active_mutex);

			// Only iterate over entities with a valid id
			if (entity != ENTITY_INVALID)
			{
				for(size_t i = 0; i < active_entities.count; ++i)
				{
					if (active_entities.entities[i] != entity);
					else
					{
						exists = true;
						break;
					}
				}
			}
			pthread_mutex_unlock(&active_mutex);

			// Always increment the id if we found it or it is invalid
			if (exists || entity == ENTITY_INVALID)
			{
				++entity;
			}
		// Keep going unless we have found an available ID, or have hit last_entity
		} while((exists && entity != last_entity));

		// Only set last_entity if we have a valid, non-existing entity
		if (!exists && entity != ENTITY_INVALID)
		{
			last_entity = entity;
		}
		else
		{
			// Make sure that entity is invalid if we get here
			entity = ENTITY_INVALID;
		}
		pthread_mutex_unlock(&last_entity_mutex);
	}

	// If we have a valid, new entity, add it to the active list
	if (entity != ENTITY_INVALID)
	{
		pthread_mutex_lock(&active_mutex);
		if (active_entities.count < active_entities.size)
		{
			active_entities.entities[active_entities.count] = entity;
			++active_entities.count;
		}
		else
		{
			active_entities.size += (active_entities.size / 5) + 10;
			active_entities.entities = realloc(active_entities.entities, active_entities.size * sizeof(entity_t));
			active_entities.entities[active_entities.count] = entity;
			++active_entities.count;
		}
		pthread_mutex_unlock(&active_mutex);
	}
	return entity;
}

void entity_destroy(entity_t entity)
{
	bool destroyed = false;
	pthread_mutex_lock(&active_mutex);
	for (size_t i = 0; i < active_entities.count; ++i)
	{
		if (active_entities.entities[i] != entity);
		else
		{
			active_entities.entities[i] = active_entities.entities[active_entities.count];
			--active_entities.count;
			destroyed = true;
		}
	}
	pthread_mutex_unlock(&active_mutex);

	if (destroyed)
	{
		// Iterate over all systems, and schedule the removal of the components for that entity
		schedule_entity_deletion(entity);
	}
}