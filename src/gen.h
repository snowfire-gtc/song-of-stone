// src/gen.h
#ifndef GEN_H
#define GEN_H

#include "common_game.h"

// Инициализация генератора шума
void gen_init_noise(uint32_t seed);

// Генерация мира по умолчанию (плоский)
WorldState gen_world_default(void);

// Генерация процедурного мира с использованием шума Перлина
WorldState gen_world_procedural(uint32_t seed, int width, int height);

// Генерация блока почвы с травой (внутренняя функция)
void gen_dirt_with_grass_internal(Block* block, int x, int y);

// Генерация дерева (ствол + листва)
void gen_tree(WorldState* world, int base_x, int base_y, int height);

// Генерация золотой жилы
void gen_gold_vein(WorldState* world, int center_x, int center_y, int radius);

// Генерация воды (заполнение нижних уровней)
void gen_water_fill(WorldState* world, int water_level);

// Инициализация тронов и флагов
void gen_thrones_and_flags(WorldState* world);

#endif // GEN_H
