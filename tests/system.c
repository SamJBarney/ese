#include "test.h"
#include "system.h"


typedef struct {
    double x;
    double y;
    double z;
} position;

int tick_count = 0;

void position_tick(entity_t entity, position * pos)
{
    ++tick_count;
}

#define ESE_SYSTEM_TYPE position
#include "system_generator.h"

int main(int arc, char ** argv)
{
    position pos = {100.0, -12.0, 123.4};
    position_functions.add(0, &pos);
    EXPECT("Component shouldn't have been added to the component array'",
        position_functions.find(0) == NULL);
    EXPECT("Component should be in the 'to add' array",
        position_pending_components.count != 0);
    position_functions.resolve();
    EXPECT("Component is in the component array",
        position_functions.find(0) != NULL);
    EXPECT("'To Add' array should be empty",
        position_pending_components.count == 0);
    position_functions.tick(0,0,1);
    EXPECT("Number of components ticked should be 1",
        tick_count == 1);
    position_functions.remove(0);
    EXPECT("Component is in the component array",
        position_functions.find(0) != NULL);
    EXPECT("'To Delete' array should have one entry",
        position_removals.count == 1);
    position_functions.resolve();
    EXPECT("Component array should be empty'",
        position_functions.find(0) == NULL);
    EXPECT("'To Delete' array should be empty",
        position_removals.count == 0);
    tick_count = 0;
    position_functions.tick(0,0,1);
    EXPECT("Number of components ticked should be 0",
        tick_count == 0);
    position_functions.add(0, &pos);
    position_functions.add(1, &pos);
    position_functions.add(2, &pos);
    position_functions.add(3, &pos);
    position_functions.add(4, &pos);
    position_functions.resolve();
    FILE * fp = fopen("position.save", "w");
    position_functions.cache(fp);
    fclose(fp);
    position_functions.remove(0);
    position_functions.remove(1);
    position_functions.remove(2);
    position_functions.remove(3);
    position_functions.remove(4);
    position_functions.resolve();
    EXPECT("After deleting all of the components, there are no entities associated with it",
        position_entities.count == 0);
    fp = fopen("position.save", "r");
    position_functions.restore(fp);
    fclose(fp);
    EXPECT("After loading a component, it has the correct number of items in it",
        position_entities.count == 5);
    return 0;
}