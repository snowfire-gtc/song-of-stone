// logic_items.h
#ifndef LOGIC_ITEMS_H
#define LOGIC_ITEMS_H

#include "common_game.h"

// Создать выпадающий предмет
void drop_item(WorldState* world, ItemType type, int amount, Team team, int x, int y);

// Inventory management
bool has_item(Character* chr, ItemType type, int amount);
void remove_item(Character* chr, ItemType type, int amount);
void add_item(Character* chr, ItemType type, int amount);

// Crafting
bool craft_item(Character* chr, ItemType output, WorldState* world);

// Block breaking
void break_block_and_drop(WorldState* world, int bx, int by, Character* breaker);

#endif // LOGIC_ITEMS_H
