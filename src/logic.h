// src/logic.h
#ifndef LOGIC_H
#define LOGIC_H

#include "common_game.h"

// Основная функция обновления игровой логики
void logic_update(WorldState* world, int frame_counter);

// Обновление персонажей
void logic_update_characters(WorldState* world, int frame_counter);

// Проверка столкновений
bool logic_check_collision(int x, int y, BlockType block);

// Получение блока по координатам
Block* logic_get_block(WorldState* world, int x, int y);

// Установка блока
void logic_set_block(WorldState* world, int x, int y, BlockType type);

#endif // LOGIC_H
