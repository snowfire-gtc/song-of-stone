#ifndef LOGIC_ARROW_H
#define LOGIC_ARROW_H

#include "common_game.h"

void logic_arrow_update_all(WorldState* world, float delta_time);
void logic_arrow_check_collisions(WorldState* world);

#endif
