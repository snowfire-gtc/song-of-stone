// src/main.c
#include "raylib.h"
#include "../common_game.h"
#include "logic/logic.h"
#include "logic/logic_warrior.h"
#include "logic/logic_archer.h"
#include "logic/logic_worker.h"
#include "logic/logic_bomb.h"
#include "logic/logic_arrow.h"
#include "draw/draw.h"
#include "draw/draw_warrior.h"
#include "draw/draw_archer.h"
#include "draw/draw_worker.h"
#include "draw/draw_bomb.h"
#include "draw/draw_arrow.h"
#include "sound/sound.h"
#include "gen/gen.h"
#include "debug/debug_console.h"

// main.c

// main.c





int main(void) {
    const int WIN_W = 1280;
    const int WIN_H = 720;
    InitWindow(WIN_W, WIN_H, "Medieval Flag CTF");
    SetTargetFPS(60);

    init_sound();
    init_ui_font();
    draw_warrior_init();
    draw_archer_init();
    draw_worker_init();
    debug_console_init();

    WorldState world = gen_world_default();

    int frame_counter = 0;
    bool debug_console_open = false;

    while (!WindowShouldClose()) {
        // Ввод
        if (IsKeyPressed(KEY_GRAVE)) {
            debug_console_open = !debug_console_open;
        }

        if (ch->type == CHAR_WORKER) {
            logic_worker_update(ch, &world, frame);
            // Пример: копание по E
            if (IsKeyPressed(KEY_E)) {
                int bx = ch->x / 16, by = ch->y / 16;
                logic_worker_dig_block(ch, &world, bx, by);
            }
        }

        // Логика
        logic_update(&world, frame_counter);
        logic_bomb_update_all(&world, frame_counter);
        logic_arrow_update_all(&world, frame_counter);
        debug_console_update(&world);

        // Отрисовка
        BeginDrawing();
        ClearBackground(RAYWHITE);
        draw_background(&world);
        draw_blocks(&world);
//        draw_characters(&world);
        draw_dropped_items(&world);
        draw_warrior_all(&world, frame_counter, world.local_player_id);
        draw_archer_all(&world, frame_counter, world.local_player_id);
        draw_worker_all(&world, frame, world.local_player_id);
        draw_bomb_all(&world, frame_counter); // передаём явно
        draw_arrow_all(&world, frame_counter);
        draw_ui(&world);

        draw_debug_vectors(&world); // поверх всего
        draw_debug_console();       // сверху

        EndDrawing();

        frame_counter++;
    }

    // Cleanup
    draw_warrior_unload();
    draw_archer_unload();
    draw_worker_unload();
    draw_bomb_unload();
    draw_arrow_unload();
//    unload_resources();
    CloseWindow();
    return 0;
}
