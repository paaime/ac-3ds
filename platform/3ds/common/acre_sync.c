#include "acre_sync.h"

AcreSync acre_sync(bool continuous, bool current, bool was_on, AcreScenes scenes)
{
    bool held = scenes.boat_ride || scenes.acre_change;
    bool value = held ? current : continuous && !scenes.first_intro;
    return (AcreSync){ value != current, value, value && !was_on };
}
