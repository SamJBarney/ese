#pragma once
#include "global.h"
#include "entity.h"

typedef enum {STOPPED, RUNNING, TICKING} ese_state_t;

typedef bool (*tick_callback_t)(uint64_t, ese_state_t);

// Sets up the ese environment
void ese_init();

// Registers a system with ese
void ese_register(const char *, void *);

// Starts the ese running
void ese_run(size_t tick_duration, tick_callback_t callback);

// Caches the current state of the system
void ese_cache();

// Loads the cached state of the system
void ese_restore();

// Inserts new components for an entity if ese hasn't started running yet
void ese_seed(const char * component, entity_t entity, void * data);

// Cleans up the ese environment
void ese_cleanup();