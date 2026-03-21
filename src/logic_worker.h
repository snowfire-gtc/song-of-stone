// logic_worker.h
#ifndef LOGIC_WORKER_H
#define LOGIC_WORKER_H

#include "../../common_game.h"

// Обновление рабочего
void logic_worker_update(Character* worker, WorldState* world, int frame_counter);

// Добыча блока (копание)
void logic_worker_dig_block(Character* worker, WorldState* world, int block_x, int block_y);

// Строительство
bool logic_worker_build_spikes(Character* worker, WorldState* world, int x, int y);
bool logic_worker_build_bridge(Character* worker, WorldState* world, int x, int y);
bool logic_worker_build_ladder(Character* worker, WorldState* world, int x, int y);
bool logic_worker_build_door(Character* worker, WorldState* world, int x, int y);

// Посадка дерева
void logic_worker_plant_tree(Character* worker, WorldState* world, int x, int y);

// Перенос почвы (быстрое перемещение)
void logic_worker_move_dirt(Character* worker, WorldState* world, int from_x, int from_y, int to_x, int to_y);

// Получение урона
void logic_worker_take_damage(Character* worker, int damage);

#endif // LOGIC_WORKER_H
