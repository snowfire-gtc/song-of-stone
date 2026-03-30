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

// Inventory management
bool has_item(Character* chr, ItemType type, int amount) {
    int current = 0;
    switch(type) {
        case ITEM_WOOD: current = chr->wood; break;
        case ITEM_STONE: current = chr->stone; break;
        case ITEM_ARROWS: current = chr->arrows; break;
        case ITEM_BOMBS: current = chr->bombs; break;
        case ITEM_COINS: current = chr->coins; break;
        default: return false;
    }
    return current >= amount;
}

void remove_item(Character* chr, ItemType type, int amount) {
    switch(type) {
        case ITEM_WOOD: chr->wood -= amount; break;
        case ITEM_STONE: chr->stone -= amount; break;
        case ITEM_ARROWS: chr->arrows -= amount; break;
        case ITEM_BOMBS: chr->bombs -= amount; break;
        case ITEM_COINS: chr->coins -= amount; break;
        default: break;
    }
}

void add_item(Character* chr, ItemType type, int amount) {
    switch(type) {
        case ITEM_WOOD: chr->wood += amount; break;
        case ITEM_STONE: chr->stone += amount; break;
        case ITEM_ARROWS: chr->arrows += amount; break;
        case ITEM_BOMBS: chr->bombs += amount; break;
        case ITEM_COINS: chr->coins += amount; break;
        default: break;
    }
}

// Crafting recipes
bool craft_item(Character* chr, ItemType output, WorldState* world) {
    // Recipes:
    // Bridge: 20 wood -> 1 bridge block
    // Ladder: 10 wood -> 1 ladder block
    // Spikes: 20 wood -> 1 spikes block
    // Door: 20 wood -> 1 door block
    // Bomb: 20 coins + 10 stone -> 1 bomb
    // Arrow pack: 10 coins + 5 wood -> 10 arrows
    
    switch(output) {
        case ITEM_WOOD: // Not craftable directly
            return false;
            
        case ITEM_STONE: // Not craftable directly
            return false;
            
        case ITEM_BOMBS:
            if (has_item(chr, ITEM_COINS, 20) && has_item(chr, ITEM_STONE, 10)) {
                remove_item(chr, ITEM_COINS, 20);
                remove_item(chr, ITEM_STONE, 10);
                add_item(chr, ITEM_BOMBS, 1);
                return true;
            }
            return false;
            
        case ITEM_ARROWS:
            if (has_item(chr, ITEM_COINS, 10) && has_item(chr, ITEM_WOOD, 5)) {
                remove_item(chr, ITEM_COINS, 10);
                remove_item(chr, ITEM_WOOD, 5);
                add_item(chr, ITEM_ARROWS, 10);
                return true;
            }
            return false;
            
        default:
            return false;
    }
}

// Block breaking and dropping items
void break_block_and_drop(WorldState* world, int bx, int by, Character* breaker) {
    Block* block = &world->blocks[by][bx];
    
    if (block->type == BLOCK_AIR || block->type == BLOCK_WATER) return;
    
    // Determine what drops
    ItemType drop_type = ITEM_STONE;
    int drop_amount = 1;
    
    switch(block->type) {
        case BLOCK_DIRT:
            // Dirt doesn't drop items usually
            break;
        case BLOCK_STONE:
            drop_type = ITEM_STONE;
            drop_amount = 1;
            if (breaker) add_item(breaker, drop_type, drop_amount);
            else drop_item(world, drop_type, drop_amount, TEAM_NONE, bx * 32, by * 32);
            break;
        case BLOCK_WOOD:
        case BLOCK_LEAFS:
            drop_type = ITEM_WOOD;
            drop_amount = 1 + (rand() % 2); // 1-2 wood
            if (breaker) add_item(breaker, drop_type, drop_amount);
            else drop_item(world, drop_type, drop_amount, TEAM_NONE, bx * 32, by * 32);
            break;
        case BLOCK_GOLD:
            drop_type = ITEM_COINS;
            drop_amount = 5 + (rand() % 5); // 5-9 coins
            if (breaker) add_item(breaker, drop_type, drop_amount);
            else drop_item(world, drop_type, drop_amount, TEAM_NONE, bx * 32, by * 32);
            break;
        default:
            break;
    }
    
    // Clear the block
    block->type = BLOCK_AIR;
    block->has_grass = false;
}
