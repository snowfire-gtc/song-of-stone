// logic_arrow.c
#include "logic_arrow.h"
#include "sound.h"
#include <math.h>

#define ARROW_GRAVITY 200.0f
#define ARROW_MAX_LIFE 5.0f

void logic_arrow_update_all(WorldState* world, float delta_time) {
    for (int i = 0; i < world->arrow_count; i++) {
        Arrow* arrow = &world->arrows[i];
        if (arrow->hit) continue;

        // Гравитация
        arrow->vy += ARROW_GRAVITY * delta_time;

        // Движение
        arrow->x += (int)(arrow->vx * delta_time);
        arrow->y += (int)(arrow->vy * delta_time);

        // Поворот (упрощённо)
        arrow->rotation = atan2f(arrow->vy, arrow->vx);

        // Проверка выхода за пределы
        if (arrow->y > WORLD_MAX_HEIGHT * 16 || arrow->x < 0 || arrow->x > WORLD_MAX_WIDTH * 16) {
            arrow->hit = true;
            continue;
        }

        // Коллизия с блоками
        int block_x = arrow->x / 16;
        int block_y = arrow->y / 16;

        if (block_x >= 0 && block_x < WORLD_MAX_WIDTH &&
            block_y >= 0 && block_y < WORLD_MAX_HEIGHT) {

            BlockType block = world->blocks[block_y][block_x].type;
            if (block != BLOCK_AIR && block != BLOCK_WATER && block != BLOCK_LEAFS) {
                arrow->hit = true;

                // Звук удара
                if (block == BLOCK_WOOD || block == BLOCK_DIRT) {
                    sound_play(SOUND_ARROW_HIT_WOOD);
                } else {
                    sound_play(SOUND_ARROW_HIT_STONE);
                }

                // Стрела застревает — можно подобрать (опционально)
                // drop_item(world, ITEM_ARROWS, 1, arrow->owner_team, arrow->x, arrow->y);
            }
        }

        // Коллизия с персонажами
        for (int j = 0; j < world->char_count; j++) {
            Character* ch = &world->characters[j];
            if (ch->hp <= 0 || ch->team == arrow->owner_team) continue;

            float dx = arrow->x - ch->x;
            float dy = arrow->y - ch->y;
            if (fabsf(dx) < 10 && fabsf(dy) < 30) {
                arrow->hit = true;
                // Урон: лучник/рабочий — 2, воин — 1 (броня щита не влияет на стрелы!)
                int damage = (ch->type == CHAR_WARRIOR) ? 1 : 2;
                // Вызов функции получения урона
                if (ch->type == CHAR_WARRIOR) {
                    extern void logic_warrior_take_damage(Character* w, int dmg, bool melee);
                    logic_warrior_take_damage(ch, damage, false); // не melee
                } else if (ch->type == CHAR_ARCHER) {
                    extern void logic_archer_take_damage(Character* a, int dmg);
                    logic_archer_take_damage(ch, damage);
                } else if (ch->type == CHAR_WORKER) {
                    ch->hp -= damage;
                    if (ch->hp < 0) ch->hp = 0;
                    sound_play(SOUND_PLAYER_HURT);
                }

                sound_play(SOUND_ARROW_HIT_CHARACTER);
                break;
            }
        }
    }
}
