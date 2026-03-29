#include "common_game.h"
#include <stdlib.h>

void drop_item(WorldState* world, ItemType type, int amount, Team team, int x, int y) {
    if (world->item_count >= MAX_PLAYERS * 4) return;
    DroppedItem* item = &world->items[world->item_count];
    item->x = x;
    item->y = y;
    item->type = type;
    item->amount = amount;
    item->team = team;
    item->vx = (rand() % 100) - 50;
    item->vy = -100;
    item->is_picked_up = false;
    world->item_count++;
}
