// logic_worker.c
#include "logic_worker.h"
#include "sound.h"
#include <math.h>

// Обновление
void logic_worker_update(Character* worker, WorldState* world, int frame_counter) {
    if (worker->is_invulnerable) {
        worker->invuln_timer -= GetFrameTime();
        if (worker->invuln_timer <= 0) {
            worker->is_invulnerable = false;
        }
    }
}

// Добыча блока
void logic_worker_dig_block(Character* worker, WorldState* world, int block_x, int block_y) {
    if (block_x < 0 || block_x >= WORLD_MAX_WIDTH ||
        block_y < 0 || block_y >= WORLD_MAX_HEIGHT) return;

    Block* block = &world->blocks[block_y][block_x];
    BlockType type = block->type;

    if (type == BLOCK_AIR || type == BLOCK_WATER || type == BLOCK_LEAFS) return;

    // Установка анимации
    worker->anim_state = ANIM_DIG;
    worker->frame_counter = 0;

    // Добыча в зависимости от типа
    if (type == BLOCK_DIRT) {
        block->type = BLOCK_AIR;
        block->has_grass = false;
        worker->wood += 1; // или можно dirt, но в инвентаре — только wood/stone
        sound_play(SOUND_DIG_GRASS);
    }
    else if (type == BLOCK_STONE) {
        // Рабочий разрушает камень мгновенно (по ТЗ)
        block->type = BLOCK_AIR;
        worker->stone += 10;
        sound_play(SOUND_DIG_STONE);
    }
    else if (type == BLOCK_GOLD) {
        block->type = BLOCK_AIR;
        worker->coins += 2;
        sound_play(SOUND_DIG_GOLD);
    }
    else if (type == BLOCK_WOOD) {
        // Рубка дерева
        block->type = BLOCK_AIR;
        worker->wood += 2;
        sound_play(SOUND_DIG_WOOD);
        // Сдвиг верхних блоков вниз
        for (int y = block_y + 1; y < WORLD_MAX_HEIGHT; y++) {
            if (world->blocks[y][block_x].type == BLOCK_WOOD) {
                world->blocks[y - 1][block_x] = world->blocks[y][block_x];
                world->blocks[y][block_x].type = BLOCK_AIR;
            } else {
                break;
            }
        }
    }
    else if (type == BLOCK_GRASS) {
        // Срезание травы
        block->has_grass = false;
        sound_play(SOUND_DIG_GRASS);
    }
}

// Строительство шипов
bool logic_worker_build_spikes(Character* worker, WorldState* world, int x, int y) {
    if (worker->wood < WORKER_BUILD_COST_SPIKES) return false;
    if (world->blocks[y][x].type != BLOCK_AIR) return false;

    worker->wood -= WORKER_BUILD_COST_SPIKES;
    world->blocks[y][x].type = BLOCK_SPIKES;
    sound_play(SOUND_BUILD);
    return true;
}

// Строительство моста
bool logic_worker_build_bridge(Character* worker, WorldState* world, int x, int y) {
    if (worker->wood < WORKER_BUILD_COST_BRIDGE) return false;
    if (world->blocks[y][x].type != BLOCK_AIR) return false;

    worker->wood -= WORKER_BUILD_COST_BRIDGE;
    world->blocks[y][x].type = BLOCK_BRIDGE;
    // Привязка к команде — можно хранить в отдельной структуре, но упростим:
    // Предположим, что мост можно убрать только тем же игроком
    sound_play(SOUND_BUILD);
    return true;
}

// Лестница
bool logic_worker_build_ladder(Character* worker, WorldState* world, int x, int y) {
    if (worker->wood < WORKER_BUILD_COST_LADDER) return false;
    if (world->blocks[y][x].type != BLOCK_AIR) return false;

    worker->wood -= WORKER_BUILD_COST_LADDER;
    world->blocks[y][x].type = BLOCK_LADDER;
    sound_play(SOUND_BUILD);
    return true;
}

// Дверь
bool logic_worker_build_door(Character* worker, WorldState* world, int x, int y) {
    if (worker->wood < WORKER_BUILD_COST_DOOR) return false;
    if (world->blocks[y][x].type != BLOCK_AIR) return false;

    worker->wood -= WORKER_BUILD_COST_DOOR;
    world->blocks[y][x].type = BLOCK_DOOR;
    sound_play(SOUND_BUILD_DOOR);
    return true;
}

// Посадка дерева
void logic_worker_plant_tree(Character* worker, WorldState* world, int x, int y) {
    if (worker->wood < 1) return;
    if (world->blocks[y][x].type != BLOCK_DIRT) return;
    if (y + 1 >= WORLD_MAX_HEIGHT) return;

    // Проверка: выше — воздух?
    if (world->blocks[y + 1][x].type != BLOCK_AIR) return;

    worker->wood -= 1;
    world->blocks[y + 1][x].type = BLOCK_WOOD; // ствол
    // Листва сверху (пример)
    if (y + 3 < WORLD_MAX_HEIGHT) {
        world->blocks[y + 3][x].type = BLOCK_LEAFS;
    }
    sound_play(SOUND_PLANT_TREE);
}

// Перенос почвы (упрощённо: мгновенно)
void logic_worker_move_dirt(Character* worker, WorldState* world, int from_x, int from_y, int to_x, int to_y) {
    if (world->blocks[from_y][from_x].type != BLOCK_DIRT) return;
    if (world->blocks[to_y][to_x].type != BLOCK_AIR) return;

    world->blocks[to_y][to_x] = world->blocks[from_y][from_x];
    world->blocks[from_y][from_x].type = BLOCK_AIR;
    world->blocks[from_y][from_x].has_grass = false;
    sound_play(SOUND_DIG_GRASS);
}

// Получение урона
void logic_worker_take_damage(Character* worker, int damage) {
    if (worker->is_invulnerable) return;
    worker->hp -= damage;
    if (worker->hp < 0) worker->hp = 0;
    sound_play(SOUND_PLAYER_HURT);
}
