// draw_worker.h
#ifndef DRAW_WORKER_H
#define DRAW_WORKER_H

#include "../../common_game.h"

void draw_worker_init(void);
void draw_worker_single(const Character* worker, int frame_counter, bool is_local);
void draw_worker_all(const WorldState* world, int frame_counter, int local_id);
void draw_worker_unload(void);

#endif // DRAW_WORKER_Hc
