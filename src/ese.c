#include "ese.h"
#include "system.h"

#include <unistd.h>
#include <dlfcn.h>
#include <errno.h>
#include <string.h>
#include <time.h>

void * tick_thread_start(void * arg);
void * resolution_thread_start(void * arg);
void ese_sleep(const struct timespec * wait_time);

typedef struct
{
    void * source;
    char * name;
    system_functions * functions;
    uint16_t uncomplete_threads;
    pthread_mutex_t mutex;
} system_wrapper;

struct
{
    size_t size;
    size_t count;
    system_wrapper * wrappers;
} system_wrapper_array = {0,0,NULL};

static struct
{
    ese_state_t current_state;
    pthread_t * tick_threads;
    pthread_t resolution_thread;
    uint64_t current_tick;
} ese_global;


static int count = 0;
uint16_t cpu_count()
{
    if (count == 0)
    {
        count = sysconf(_SC_NPROCESSORS_ONLN);
        if (count < 0)
        {
            exit (errno);
        }
    }
    return count;
}



void ese_init()
{
    ese_global.tick_threads = NULL;
    ese_global.current_state = STOPPED;
    ese_global.current_tick = 0;

}



void ese_register(const char * name, void * source)
{
    size_t name_length = strlen(name);
    // Build the symbol name
    char * symbol = malloc(name_length + strlen("_functions") + 1);
    strcpy(symbol, name);
    strcat(symbol, "_functions");
    // Find the symbol
    void * functions = dlsym(source, symbol);
    // Release the extra memory we acquired
    free(symbol);
    if (functions)
    {
        if(system_wrapper_array.count < system_wrapper_array.size);
        else
        {
            system_wrapper_array.size += 2;
            system_wrapper_array.wrappers = realloc(system_wrapper_array.wrappers, sizeof(system_wrapper) * system_wrapper_array.size);
        }
        // Add the source
        system_wrapper_array.wrappers[system_wrapper_array.count].source = source;

        // Copy the name
        system_wrapper_array.wrappers[system_wrapper_array.count].name = malloc(name_length + 1);
        memset(system_wrapper_array.wrappers[system_wrapper_array.count].name, '\0', name_length + 1);
        strcpy(system_wrapper_array.wrappers[system_wrapper_array.count].name, name);

        // Add the functions
        system_wrapper_array.wrappers[system_wrapper_array.count].functions = (system_functions *)functions;

        // Set the ticking_count to complete
        system_wrapper_array.wrappers[system_wrapper_array.count].uncomplete_threads = cpu_count();

        // Initialize the mutex for ticking_count
        pthread_mutex_init(&(system_wrapper_array.wrappers[system_wrapper_array.count].mutex), NULL);
        ++system_wrapper_array.count;
    }
}



void ese_run(size_t tick_duration, tick_callback_t callback)
{
    // Start tick_threads
    uint64_t count = cpu_count();
    ese_global.tick_threads = malloc(sizeof(ese_global.tick_threads) * count);
    for (uint64_t i = 0; i < count; ++i)
    {
        pthread_create((ese_global.tick_threads + i), NULL, tick_thread_start, (void*)i);
    }
    pthread_create(&ese_global.resolution_thread, NULL, resolution_thread_start, (void*)count);

    // Actually run things
    struct timespec wait_time = {0, tick_duration * 1000};
    __atomic_store_n(&ese_global.current_state, RUNNING, __ATOMIC_RELEASE);
    while (__atomic_load_n(&ese_global.current_state, __ATOMIC_ACQUIRE) < TICKING);
    while (__atomic_load_n(&ese_global.current_state, __ATOMIC_ACQUIRE) > RUNNING)
    {
        ese_sleep(&wait_time);
        uint64_t current_tick = __atomic_load_n(&ese_global.current_tick, __ATOMIC_ACQUIRE);
        ese_state_t current_state = __atomic_load_n(&ese_global.current_state, __ATOMIC_ACQUIRE);
        if (callback(current_tick, current_state));
        else
        {
            break;
        }
    }

    // Cleanup threads
    __atomic_store_n(&ese_global.current_state, STOPPED, __ATOMIC_RELEASE);
    for (size_t i = 0; i < cpu_count(); ++i)
    {
        pthread_join(ese_global.tick_threads[i], NULL);
    }
    pthread_join(ese_global.resolution_thread, NULL);

    free(ese_global.tick_threads);
    ese_global.tick_threads = NULL;
}



void ese_cache()
{

}



void ese_restore()
{

}



void ese_seed(const char * component, entity_t entity, void * data)
{
    if (__atomic_load_n(&ese_global.current_state, __ATOMIC_ACQUIRE) > STOPPED)
    {
        for(size_t i = 0; i < system_wrapper_array.count; ++i)
        {
            if (strcmp(system_wrapper_array.wrappers[i].name, component) == 0)
            {
                system_wrapper_array.wrappers[i].functions->add(entity, data);
                system_wrapper_array.wrappers[i].functions->resolve();
                break;
            }
        }
    }
}



void * tick_thread_start(void * arg)
{
    uint16_t thread_id = (uint64_t)arg;
    while (__atomic_load_n(&ese_global.current_state, __ATOMIC_RELAXED) == STOPPED);
    while (__atomic_load_n(&ese_global.current_state, __ATOMIC_ACQUIRE) > STOPPED)
    {
        if (__atomic_load_n(&ese_global.current_state, __ATOMIC_ACQUIRE) == TICKING)
        {
            for (size_t i = 0; i < system_wrapper_array.count; ++i)
            {
                while (__atomic_load_n(&(system_wrapper_array.wrappers[i].uncomplete_threads), __ATOMIC_RELAXED) == 0);
                system_wrapper_array.wrappers[i].functions->tick(ese_global.current_tick, thread_id, cpu_count());
                pthread_mutex_lock(&system_wrapper_array.wrappers[i].mutex);
                __atomic_sub_fetch(&(system_wrapper_array.wrappers[i].uncomplete_threads), 1, __ATOMIC_ACQ_REL);
                pthread_mutex_unlock(&system_wrapper_array.wrappers[i].mutex);
            }
            while (__atomic_load_n(&(ese_global.current_state), __ATOMIC_RELAXED) == TICKING);
        }
    }
    return NULL;
}



void * resolution_thread_start(void * arg)
{
    while (__atomic_load_n(&ese_global.current_state, __ATOMIC_RELAXED) == STOPPED);
    while (__atomic_load_n(&ese_global.current_state, __ATOMIC_ACQUIRE) > STOPPED)
    {
        if (__atomic_load_n(&ese_global.current_state, __ATOMIC_ACQUIRE) == TICKING)
        {
            for (size_t i = 0; i < system_wrapper_array.count; ++i)
            {
                while (__atomic_load_n(&(system_wrapper_array.wrappers[i].uncomplete_threads), __ATOMIC_RELAXED) != 0);
                system_wrapper_array.wrappers[i].functions->resolve();
                pthread_mutex_lock(&system_wrapper_array.wrappers[i].mutex);
                __atomic_store_n(&(system_wrapper_array.wrappers[i].uncomplete_threads), cpu_count(), __ATOMIC_RELEASE);
                pthread_mutex_unlock(&system_wrapper_array.wrappers[i].mutex);
            }
            ese_state_t expected = TICKING;
            __atomic_add_fetch(&ese_global.current_tick, 1, __ATOMIC_ACQ_REL);
            __atomic_compare_exchange_n(&ese_global.current_state, &expected, RUNNING, false, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED);
        }
    }
    return NULL;
}



void ese_sleep(const struct timespec * wait_time)
{
    struct timespec current = *wait_time;
    struct timespec rem = {0,0};
    while(nanosleep(&current, &rem) == -1)
    {
        current = rem;
        rem.tv_sec = 0;
        rem.tv_nsec = 0;
    }
}