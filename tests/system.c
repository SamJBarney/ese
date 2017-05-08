#include "test.h"
#include "system.h"


typedef struct {
    double x;
    double y;
    double z;
} position;


int tick_count = 0;

#define ESE_SYSTEM_TICK position_tick
void position_tick(entity e, position * pos)
{
    ++tick_count;
}

#define ESE_SYSTEM_TYPE position
#include "system_generator.h"

int main(int arc, char ** argv)
{
    position pos = {100.0, -12.0, 123.4};
    position_system.add(0, &pos);
    EXPECT("Component shouldn't have been added to the component array'",
        position_system.find(0) == NULL);
    EXPECT("Component should be in the 'to add' array",
        position_pending_components.count != 0);
    position_system.resolve();
    EXPECT("Component is in the component array",
        position_system.find(0) != NULL);
    EXPECT("'To Add' array should be empty",
        position_pending_components.count == 0);
    position_system.tick(0,0,1);
    EXPECT("Number of components ticked should be 1",
        tick_count == 1);
    position_system.remove(0);
    EXPECT("Component is in the component array",
        position_system.find(0) != NULL);
    EXPECT("'To Delete' array should have one entry",
        position_removals.count == 1);
    position_system.resolve();
    EXPECT("Component array should be empty'",
        position_system.find(0) == NULL);
    EXPECT("'To Delete' array should be empty",
        position_removals.count == 0);
    tick_count = 0;
    position_system.tick(0,0,1);
    EXPECT("Number of components ticked should be 0",
        tick_count == 0);
    return 0;
}