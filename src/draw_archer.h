// draw_archer.h
#ifndef DRAW_ARCHER_H
#define DRAW_ARCHER_H

#include "common_game.h"

// Инициализация текстур лучника
void draw_archer_init(void);

// Отрисовка одного лучника
void draw_archer_single(const Character* archer, int frame_counter, bool is_local_player);

// Отрисовка всех лучников
void draw_archer_all(const WorldState* world, int frame_counter, int local_player_id);

// Освобождение ресурсов
void draw_archer_unload(void);

#endif // DRAW_ARCHER_H
