Entity System Engine
====================

## Features
- Programmer doesn't need to worry about threading (all handled within the engine itself)
- Writing a new system is as simple as including the system_generator header
- Adding components to entities is as simple as calling the associated function handle

## API
### bool ese_register(const char * name, system_t * system)
> This function adds a system to the engine to be managed.

### void ese_seed(const char * system, entity e, void * data)
> Allows components to be added to systems before the engine is started

### void ese_run(size_t tick_duration, tick_callback_t callback)
> Starts the engine running. Calls the callback after tick_duration milliseconds.

### entity entity_create()
> Creates a new unique entity that can be used in the engine.

### entity entity_create_if_exists(entity)
> Returns the passed in entity if it doesn't exist in the engine yet. If it already exists it returns a new entity.

### void entity_destroy(entity)
> Removes an entity from the engine, marking the associated components as deleteable

## Example System
    #include "system.h"
    
    #define ESE_SYSTEM_TYPE example
    typedef struct
    {
      int value;
    } example;
    
    #define ESE_ADD_HOOK add_hook
    void add_hook(entity e, example * component)
    {
      // Allows you to alter the component before it is added to the system
    }
    
    #define ESE_DELETE_HOOK delete_hook
    void delete_hook(entity e, example * component)
    {
    }
    
    #include "system_generator.h"
