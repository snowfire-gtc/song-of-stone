// draw_arrow.h
#ifndef DRAW_ARROW_H
#define DRAW_ARROW_H

#include "common_game.h"

void draw_arrow_init(void);
void draw_arrow_all(const WorldState* world, int frame_counter);
void draw_arrow_unload(void);

#endif // DRAW_ARROW_H
