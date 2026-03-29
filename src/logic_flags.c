// Проверка: игрок стоит на флаге?
#include "common_game.h"
#include "sound.h"
#include <stdbool.h>
#include <stdlib.h>

bool is_player_on_flag(const Character* ch, Vector2 flag_pos) {
    int dx = abs(ch->x - (int)flag_pos.x);
    int dy = abs(ch->y - (int)flag_pos.y);
    return (dx < 16 && dy < 32);
}

void logic_update_flags(WorldState* world) {
    // Если флаг не захвачен — проверяем поднятие
    if (!world->flag_blue_carried) {
        for (int i = 0; i < world->char_count; i++) {
            Character* ch = &world->characters[i];
            if (ch->team == TEAM_RED && is_player_on_flag(ch, world->flag_blue_pos)) {
                world->flag_blue_carried = true;
                ch->is_holding_flag = true;
                world->flag_carrier_id = ch->player_id;
                sound_play(SOUND_FLAG_CAPTURE);
            }
        }
    }

    // Если игрок с флагом умирает — флаг падает
    if (world->flag_blue_carried) {
        Character* carrier = NULL;
        for (int i = 0; i < world->char_count; i++) {
            if (world->characters[i].player_id == world->flag_carrier_id) {
                carrier = &world->characters[i];
                break;
            }
        }
        if (!carrier || carrier->hp <= 0) {
            // Выпускаем флаг
            // drop_item(world, ITEM_FLAG, 1, TEAM_BLUE, carrier ? carrier->x : (int)world->flag_blue_pos.x, carrier ? carrier->y : (int)world->flag_blue_pos.y);
            world->flag_blue_carried = false;
            world->flag_carrier_id = -1;
            if (carrier) carrier->is_holding_flag = false;
        }
    }
}
