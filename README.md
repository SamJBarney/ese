Entity System Engine
====================

## Features
- Programmer doesn't need to worry about threading (all handled within the engine itself)
- Writing a new system is as simple as including the system_generator header
- 

## API
### bool ese_register(const char * name, system_t * system)
### void ese_seed(const char * system, entity e, void * data)
### void ese_run(size_t tick_duration, tick_callback_t callback)
### entity entity_create()
### void entity_destroy(entity)