#include "ese.h"
#include "system.h"

#include <unistd.h>
#include <dlfcn.h>
#include <errno.h>
#include <string.h>
#include <time.h>

void * tick_thread_start(void * arg);
void ese_sleep(const struct timespec * wait_time);

typedef struct
{
    char * name;
    system_t * system;
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
    uint64_t current_tick;
    uint16_t tick_complete;
    uint16_t resolution_threads;
    pthread_mutex_t mutex;
} ese_global = {STOPPED, NULL, 0, 0, 0};


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



bool ese_register(const char * name, system_t * system)
{
    // Make sure the system hasn't been registered before
    for (size_t i = 0; i < system_wrapper_array.count; ++i)
    {
        if (strcmp(system_wrapper_array.wrappers[i].name, name) != 0);
        else
        {
            return false;
        }
    }

    if(system_wrapper_array.count < system_wrapper_array.size);
    else
    {
        system_wrapper_array.size += 2;
        system_wrapper_array.wrappers = realloc(system_wrapper_array.wrappers, sizeof(system_wrapper) * system_wrapper_array.size);
    }

    // Copy the name
    size_t name_length = strlen(name);
    system_wrapper_array.wrappers[system_wrapper_array.count].name = malloc(name_length + 1);
    memset(system_wrapper_array.wrappers[system_wrapper_array.count].name, '\0', name_length + 1);
    strcpy(system_wrapper_array.wrappers[system_wrapper_array.count].name, name);

    // Add the functions
    system_wrapper_array.wrappers[system_wrapper_array.count].system = system;

    ++system_wrapper_array.count;
    return true;
}



void ese_run(size_t tick_duration, tick_callback_t callback)
{
    pthread_mutex_init(&ese_global.mutex, NULL);
    // Start tick_threads
    uint64_t count = cpu_count();
    ese_global.tick_threads = malloc(sizeof(ese_global.tick_threads) * count);
    for (uint64_t i = 0; i < count; ++i)
    {
        pthread_create((ese_global.tick_threads + i), NULL, tick_thread_start, (void*)i);
    }

    // Actually run things
    struct timespec wait_time = {0, tick_duration * 1000};
    __atomic_store_n(&ese_global.current_state, TICKING, __ATOMIC_RELEASE);
    while (true)
    {
        ese_sleep(&wait_time);
        uint64_t current_tick = __atomic_load_n(&ese_global.current_tick, __ATOMIC_ACQUIRE);
        ese_state_t current_state = __atomic_load_n(&ese_global.current_state, __ATOMIC_ACQUIRE);
        if (callback(current_tick, current_state));
        else
        {
            break;
        }
        if (current_state != TICKING)
        {
            __atomic_add_fetch(&ese_global.current_tick, 1, __ATOMIC_ACQ_REL);
        }
    }
    // Cleanup threads
    __atomic_store_n(&ese_global.current_state, STOPPED, __ATOMIC_RELEASE);
    for (size_t i = 0; i < cpu_count(); ++i)
    {
        pthread_join(ese_global.tick_threads[i], NULL);
    }
    free(ese_global.tick_threads);
    ese_global.tick_threads = NULL;
}



void ese_seed(const char * system, entity entity, void * data)
{
    if (__atomic_load_n(&ese_global.current_state, __ATOMIC_ACQUIRE) > STOPPED)
    {
        for(size_t i = 0; i < system_wrapper_array.count; ++i)
        {
            if (strcmp(system_wrapper_array.wrappers[i].name, system) == 0)
            {
                system_wrapper_array.wrappers[i].system->add(entity, data);
                system_wrapper_array.wrappers[i].system->resolve();
                break;
            }
        }
    }
}



void resolve_systems(uint16_t thread_id, uint16_t thread_count)
{
    // Figure out how many systems each thread will be resolving, min 1
    size_t count = system_wrapper_array.count / thread_count + 1;
    size_t start = count * thread_id;
    size_t end = start + count;
    for (size_t i = start; i < end && i < system_wrapper_array.count; ++i)
    {
        system_wrapper_array.wrappers[i].system->resolve();
    }
}



void * tick_thread_start(void * arg)
{
    uint16_t thread_id = (uint64_t)arg;

    // Wait until things are started
    while (__atomic_load_n(&ese_global.current_state, __ATOMIC_RELAXED) == STOPPED);

    // While the thread hasn't been told to stop
    while (__atomic_load_n(&ese_global.current_state, __ATOMIC_ACQUIRE) > STOPPED)
    {
        // If the thread has been told to tick
        if (__atomic_load_n(&ese_global.current_state, __ATOMIC_ACQUIRE) == TICKING)
        {

            for (size_t i = 0; i < system_wrapper_array.count; ++i)
            {
                system_wrapper_array.wrappers[i].system->tick(ese_global.current_tick, thread_id, cpu_count());
            }
            // Increment the number of tick complete threads
            pthread_mutex_lock(&ese_global.mutex);
            __atomic_add_fetch(&ese_global.tick_complete, 1, __ATOMIC_ACQ_REL);
            pthread_mutex_unlock(&ese_global.mutex);

            // Wait for all threads to have completed
            while (__atomic_load_n(&(ese_global.tick_complete), __ATOMIC_RELAXED) != 8);

            // Increment the number of resolution threads
            pthread_mutex_lock(&ese_global.mutex);
            __atomic_add_fetch(&ese_global.resolution_threads, 1, __ATOMIC_ACQ_REL);
            pthread_mutex_unlock(&ese_global.mutex);

            // Resolve systems for this thread
            resolve_systems(thread_id, cpu_count());

            while (__atomic_load_n(&ese_global.resolution_threads, __ATOMIC_RELAXED) != 8);

            // Set the current state to RUNNING
            __atomic_store_n(&ese_global.current_state, RUNNING, __ATOMIC_RELEASE);

            // Decrement the number of tick complete threads
            pthread_mutex_lock(&ese_global.mutex);
            __atomic_sub_fetch(&ese_global.tick_complete, 1, __ATOMIC_ACQ_REL);
            pthread_mutex_unlock(&ese_global.mutex);

            // Wait for idle threads to be zero
            while (__atomic_load_n(&(ese_global.tick_complete), __ATOMIC_RELAXED) != 0);

            // Decrement number of resolution threads
            pthread_mutex_lock(&ese_global.mutex);
            __atomic_sub_fetch(&ese_global.resolution_threads, 1, __ATOMIC_ACQ_REL);
            pthread_mutex_unlock(&ese_global.mutex);

            // Wait for all threads to have completed resolving
            while (__atomic_load_n(&(ese_global.resolution_threads), __ATOMIC_RELAXED) != 0);
        }
    }
    return NULL;
}



// Internal
void ese_sleep(const struct timespec * wait_time)
{
    struct timespec current = *wait_time;
    struct timespec rem = {0,0};
    
    // Sleep until all time asked to sleep has been completed
    while(nanosleep(&current, &rem) == -1)
    {
        current = rem;
        rem.tv_sec = 0;
        rem.tv_nsec = 0;
    }
}



void schedule_entity_deletion(entity e)
{
    for (size_t i = 0; i < system_wrapper_array.count; ++i)
    {
        system_wrapper_array.wrappers[i].system->remove(e);
    }
}