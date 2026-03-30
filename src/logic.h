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

// Физика блоков
void logic_update_block_physics(WorldState* world);
void logic_update_falling_blocks(WorldState* world);
void logic_update_sliding_blocks(WorldState* world);
bool is_block_fallable(BlockType type);
bool is_block_solid(BlockType type);

// Инвентарь и предметы (из logic_items.h)
void drop_item(WorldState* world, ItemType type, int amount, Team team, int x, int y);
bool has_item(Character* chr, ItemType type, int amount);
void remove_item(Character* chr, ItemType type, int amount);
void add_item(Character* chr, ItemType type, int amount);
bool craft_item(Character* chr, ItemType output, WorldState* world);
void break_block_and_drop(WorldState* world, int bx, int by, Character* breaker);

#endif // LOGIC_H
