// draw_warrior.h
#ifndef DRAW_WARRIOR_H
#define DRAW_WARRIOR_H

#include "../../common_game.h"

// Инициализация текстур воина
void draw_warrior_init(void);

// Отрисовка одного воина
void draw_warrior_single(const Character* warrior, int frame_counter, bool is_local_player);

// Отрисовка всех воинов
void draw_warrior_all(const WorldState* world, int frame_counter, int local_player_id);

// Освобождение ресурсов
void draw_warrior_unload(void);

#endif // DRAW_WARRIOR_H
