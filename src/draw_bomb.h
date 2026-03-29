// draw_bomb.h
#ifndef DRAW_BOMB_H
#define DRAW_BOMB_H

#include "common_game.h"

// Инициализация текстур бомбы
void draw_bomb_init(void);

// Отрисовка одной бомбы
void draw_bomb_single(const Bomb* bomb, int frame_counter);

// Отрисовка всех бомб
void draw_bomb_all(const WorldState* world, int frame_counter);

// Освобождение ресурсов
void draw_bomb_unload(void);

#endif // DRAW_BOMB_H
