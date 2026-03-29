// logic_bomb.h
#ifndef LOGIC_BOMB_H
#define LOGIC_BOMB_H

#include "common_game.h"

// Создать бомбу (бросок воином)
Bomb logic_bomb_create(int x, int y, float vx, float vy, int owner_id, Team owner_team, float fuse_time);

// Обновить все бомбы
void logic_bomb_update_all(WorldState* world, float delta_time);

// Взрыв бомбы
void logic_bomb_explode(WorldState* world, int bomb_index);

// Проверка коллизии бомбы с блоками и персонажами
void logic_bomb_check_collisions(WorldState* world, int bomb_index);

// Разрушение блоков в радиусе
void logic_bomb_destroy_blocks(WorldState* world, int center_x, int center_y, int radius);

// Нанесение урона персонажам в радиусе
void logic_bomb_damage_characters(WorldState* world, int center_x, int center_y, int radius, int owner_id);

#endif // LOGIC_BOMB_H
