// src/logic/logic_physics.c
bool is_in_water(const WorldState* world, int x, int y) {
    // Персонаж 2 блока высотой
    return (world->blocks[y][x].type == BLOCK_WATER) &&
           (world->blocks[y + 1][x].type == BLOCK_WATER);
}

void logic_update_oxygen_system(WorldState* world) {
    for (int i = 0; i < world->char_count; i++) {
        Character* ch = &world->characters[i];
        int x = ch->x / 16; // предположим, блок = 16x16 пикс.
        int y = ch->y / 16;

        if (is_in_water(world, x, y)) {
            ch->oxygen -= 1;
            if (ch->oxygen <= 0) {
                ch->hp = 0; // утонул
            }
        } else {
            if (ch->oxygen < PLAYER_OXYGEN_MAX) {
                ch->oxygen++;
            }
        }
    }
}
