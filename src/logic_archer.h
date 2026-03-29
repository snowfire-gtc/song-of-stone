// logic_archer.h
#ifndef LOGIC_ARCHER_H
#define LOGIC_ARCHER_H

#include "common_game.h"

// Обновление лучника
void logic_archer_update(Character* archer, WorldState* world, int frame_counter);

// Стрельба
void logic_archer_shoot_arrow(Character* archer, WorldState* world, float power);

// Добыча стрел из дерева или листвы
void logic_archer_harvest_arrows(Character* archer, WorldState* world);

// Проверка: может ли лучник лезть?
bool logic_archer_can_climb_block(BlockType block_type);

// Обновление лазания
void logic_archer_update_climbing(Character* archer, WorldState* world);

// Получение урона (у лучника нет щита)
void logic_archer_take_damage(Character* archer, int damage);

// Проверка: падение на листву → смягчение
bool logic_archer_survive_fall_on_leafs(Character* archer, WorldState* world, int fall_height);

#endif // LOGIC_ARCHER_H
