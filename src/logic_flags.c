// logic_flags.c
#include "common_game.h"
#include "sound.h"
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>

bool is_player_on_flag(const Character* ch, Vector2 flag_pos) {
    int dx = abs(ch->x - (int)flag_pos.x);
    int dy = abs(ch->y - (int)flag_pos.y);
    return (dx < 16 && dy < 32);
}

bool is_player_near_throne(const Character* ch, Vector2 throne_pos, Team team) {
    (void)team; // пока не используется, но может понадобиться для разных радиусов
    int dx = abs(ch->x - (int)throne_pos.x);
    int dy = abs(ch->y - (int)throne_pos.y);
    return (dx < FLAG_RETURN_RADIUS * 16 && dy < FLAG_RETURN_RADIUS * 16);
}

void logic_update_flags(WorldState* world) {
    // Обновляем анимацию флага
    world->flag_anim.wave_offset += 0.05f;
    if (world->flag_anim.wave_offset > 2.0f * PI) {
        world->flag_anim.wave_offset -= 2.0f * PI;
    }
    
    // Сила ветра зависит от времени (циклически)
    world->flag_anim.wind_strength = 0.5f + 0.5f * sinf(world->flag_anim.wave_offset * 0.5f);
    
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
    
    // Аналогично для красного флага
    if (!world->flag_red_carried) {
        for (int i = 0; i < world->char_count; i++) {
            Character* ch = &world->characters[i];
            if (ch->team == TEAM_BLUE && is_player_on_flag(ch, world->flag_red_pos)) {
                world->flag_red_carried = true;
                ch->is_holding_flag = true;
                world->flag_carrier_id = ch->player_id;
                sound_play(SOUND_FLAG_CAPTURE);
            }
        }
    }

    // Если игрок с флагом умирает — флаг падает
    if (world->flag_blue_carried || world->flag_red_carried) {
        Character* carrier = NULL;
        for (int i = 0; i < world->char_count; i++) {
            if (world->characters[i].player_id == world->flag_carrier_id) {
                carrier = &world->characters[i];
                break;
            }
        }
        
        bool flag_dropped = false;
        Team dropped_team = TEAM_NONE;
        
        if (!carrier || carrier->hp <= 0) {
            flag_dropped = true;
            dropped_team = world->flag_blue_carried ? TEAM_BLUE : TEAM_RED;
        }
        
        // Проверяем, достиг ли игрок с флагом трона своей команды
        if (carrier && carrier->is_holding_flag) {
            Vector2 own_throne = (carrier->team == TEAM_BLUE) ? world->throne_blue : world->throne_red;
            
            if (is_player_near_throne(carrier, own_throne, carrier->team)) {
                // Захват флага завершён!
                if (carrier->team == TEAM_BLUE) {
                    world->params.blue_score += 1;
                    world->flag_anim.capture_progress = 1.0f;
                } else {
                    world->params.red_score += 1;
                    world->flag_anim.capture_progress = 1.0f;
                }
                
                // Возвращаем флаг на место
                world->flag_blue_carried = false;
                world->flag_red_carried = false;
                world->flag_carrier_id = -1;
                carrier->is_holding_flag = false;
                sound_play(SOUND_FLAG_SCORE);
                
                // Сбрасываем прогресс через некоторое время
                world->flag_anim.capture_progress = 0.0f;
            }
        }
        
        if (flag_dropped) {
            // Флаг упал (игрок умер)
            if (dropped_team == TEAM_BLUE) {
                world->flag_blue_carried = false;
            } else {
                world->flag_red_carried = false;
            }
            world->flag_carrier_id = -1;
            if (carrier) {
                carrier->is_holding_flag = false;
            }
            sound_play(SOUND_FLAG_DROP);
        }
    }
}
