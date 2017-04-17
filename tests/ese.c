#include "test.h"

#include "../src/ese.c"

int test_cpu_count()
{
    EXPECT("CPU Count should not be zero", cpu_count() != 0);
    return 0;
}



void * ese_system = NULL;
int test_ese_register()
{
    EXPECT("Loaded library is not null", ese_system != NULL);
    ese_register("something", ese_system);
    EXPECT("Registering a system works", system_wrapper_array.count == 1);
    return 0;
}



int main(int argc, char ** argv)
{
    test_cpu_count();
    ese_system = dlopen("files/something.system", RTLD_LAZY);
    if(!test_ese_register())
    {
        free(system_wrapper_array.wrappers);
        system_wrapper_array.count = 0;
        system_wrapper_array.size = 0;
    }
    dlclose(ese_system);
    return 1;
}