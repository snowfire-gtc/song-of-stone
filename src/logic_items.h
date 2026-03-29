// logic_items.h
#ifndef LOGIC_ITEMS_H
#define LOGIC_ITEMS_H

#include "common_game.h"

// Создать выпадающий предмет
void drop_item(WorldState* world, ItemType type, int amount, Team team, int x, int y);

#endif // LOGIC_ITEMS_H
