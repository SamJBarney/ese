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

## Creating a system
Creating a new system is rather easy as ese generates the necessary functions for the system (add, delete, etc.) so you don't have to write them yourself. However, hooks are provided to allow the programmer to add additional functionality into those generated functions as needed. How to use the hooks can be seen in the example at the bottom.

### Hooks
#### ESE_ADD_HOOK
> Hooks into the system's add function. Gets passed the entity id and the pointer to the component being added prior to addition. Returns void.

#### ESE_DELETE_HOOK
> Hooks into the system's delete function. Gets passed the entity id and the pointer to the component being removed prior to removal. Returns void.

#### ESE_RESOLVE_HOOK
> Hooks into the system's resolution function. No arguments. Called before the adding and removal of components. Returns void.

#### ESE_SYSTEM_TICK
> The function that is called during the tick phase of the engine. Gets passed the entity id and a pointer to the associated component. Returns void.


### Example System
    #include "system.h"
    
    #define ESE_SYSTEM_TYPE example
    typedef struct
    {
      int value;
    } example;
    
    #define ESE_ADD_HOOK add_hook // Doesn't need to be defined unless you want to hook into the add functionality
    void add_hook(entity e, example * component)
    {
      // Do stuff here
    }
    
    #define ESE_DELETE_HOOK delete_hook // Doesn't need to be defined unless you want to hook into the delete functionality
    void delete_hook(entity e, example * component)
    {
        // Do stuff here
    }
    
    #define ESE_RESOLVE_HOOK resolve_hook
    void resolve_hook()
    {
        // Do stuff here
    }
    
    #define ESE_SYSTEM_TICK example_tick // Doesn't need to be defined unless you want to do something to the component each tick
    void example_tick(entity e, example * component)
    {
        // Do stuff here
    }
    
    #include "system_generator.h"
