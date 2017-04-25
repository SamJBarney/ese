#include "test.h"
#include "system.h"


typedef struct {
    double x;
    double y;
    double z;
} position;


int tick_count = 0;

#define ESE_SYSTEM_TIC position_tick
void position_tick(entity e, position * pos)
{
    ++tick_count;
}

#define ESE_SYSTEM_TYPE position
#include "system_generator.h"

int main(int arc, char ** argv)
{
    position pos = {100.0, -12.0, 123.4};
    functions.add(0, &pos);
    EXPECT("Component shouldn't have been added to the component array'",
        functions.find(0) == NULL);
    EXPECT("Component should be in the 'to add' array",
        pending_components.count != 0);
    functions.resolve();
    EXPECT("Component is in the component array",
        functions.find(0) != NULL);
    EXPECT("'To Add' array should be empty",
        pending_components.count == 0);
    functions.tick(0,0,1);
    EXPECT("Number of components ticked should be 1",
        tick_count == 1);
    functions.remove(0);
    EXPECT("Component is in the component array",
        functions.find(0) != NULL);
    EXPECT("'To Delete' array should have one entry",
        removals.count == 1);
    functions.resolve();
    EXPECT("Component array should be empty'",
        functions.find(0) == NULL);
    EXPECT("'To Delete' array should be empty",
        removals.count == 0);
    tick_count = 0;
    functions.tick(0,0,1);
    EXPECT("Number of components ticked should be 0",
        tick_count == 0);
    return 0;
}