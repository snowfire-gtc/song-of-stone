// logic_warrior.h
#ifndef LOGIC_WARRIOR_H
#define LOGIC_WARRIOR_H

#include "../../common_game.h"

// Обновление состояния воина (вызывается каждый кадр)
void logic_warrior_update(Character* warrior, WorldState* world, int frame_counter);

// Атака мечом (обычная или усиленная)
void logic_warrior_perform_sword_attack(Character* warrior, WorldState* world);

// Бросок бомбы
void logic_warrior_throw_bomb(Character* warrior, WorldState* world, float throw_power);

// Активация/деактивация щита
void logic_warrior_toggle_shield(Character* warrior);

// Проверка: может ли воин пройти через блок
bool logic_warrior_can_pass_block(BlockType block_type);

// Обработка урона (с учётом брони щита)
void logic_warrior_take_damage(Character* warrior, int damage, bool is_melee);

// Rocket jump от взрыва бомбы
void logic_warrior_rocketjump_from_bomb(Character* warrior, Vector2 bomb_pos);

#endif // LOGIC_WARRIOR_H
