#pragma once
#include "global.h"
#include "system.h"

typedef enum {STOPPED, RUNNING, TICKING} ese_state_t;

typedef bool (*tick_callback_t)(uint64_t, ese_state_t);

// Registers a system with ese
bool ese_register(const char * name, system_t * system);

// Starts the ese running
void ese_run(size_t tick_duration, tick_callback_t callback);

// Inserts new components for an entity if ese hasn't started running yet
void ese_seed(const char * system, entity e, void * data);