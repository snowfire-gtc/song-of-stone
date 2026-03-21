void logic_update_characters() {
    // В logic_update_characters()
    if (ch->type == CHAR_WARRIOR) {
        logic_warrior_update(ch, world, frame_counter);

        // Пример: если нажата клавиша АТАКИ
        if (IsKeyPressed(KEY_SPACE)) {
            logic_warrior_perform_sword_attack(ch, world);
        }

        // Если нажата клавиша ЩИТА
        if (IsKeyPressed(KEY_LEFT_CONTROL)) {
            logic_warrior_toggle_shield(ch);
        }

        // Если заряжается бросок
        if (IsKeyDown(KEY_X)) {
            ch->is_charging = true;
        }
        if (IsKeyReleased(KEY_X)) {
            logic_warrior_throw_bomb(ch, world, ch->charge_time);
            ch->is_charging = false;
            ch->charge_time = 0;
        }
    }
    // В logic_update_characters()
    if (ch->type == CHAR_ARCHER) {
        logic_archer_update(ch, world, frame_counter);

        // Натяжка лука
        if (IsKeyDown(KEY_SPACE)) {
            ch->is_aiming = true;
        }
        if (IsKeyReleased(KEY_SPACE)) {
            logic_archer_shoot_arrow(ch, world, ch->aim_time);
        }

        // Добыча стрел (удар — например, клавиша E)
        if (IsKeyPressed(KEY_E)) {
            logic_archer_harvest_arrows(ch, world);
        }

        // Начало лазания (если рядом дерево/стена и нажат W/A/S/D + SHIFT)
        if (IsKeyDown(KEY_LEFT_SHIFT)) {
            int x = ch->x / 16;
            int y = ch->y / 16;
            if (logic_archer_can_climb_block(world->blocks[y][x].type)) {
                ch->is_climbing = true;
                ch->climbing_block_x = x;
            }
        }
    }
}


// В logic_update()
void logic_update(WorldState* world, int frame_counter) {
    float delta_time = 1.0f / 60.0f; // или GetFrameTime()

    // Обновление бомб
    logic_bomb_update_all(world, delta_time);

    // Удаление взорванных бомб (или пометка)
    // (можно добавить "время жизни после взрыва" для визуала)
}
