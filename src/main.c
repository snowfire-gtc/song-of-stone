// src/main.c
#include "raylib.h"
#include "common_game.h"
#include "logic.h"
#include "logic_warrior.h"
#include "logic_archer.h"
#include "logic_worker.h"
#include "logic_bomb.h"
#include "logic_arrow.h"
#include "draw.h"
#include "draw_warrior.h"
#include "draw_archer.h"
#include "draw_worker.h"
#include "draw_bomb.h"
#include "draw_arrow.h"
#include "sound.h"
#include "gen.h"
#include "debug_console.h"

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
    Character* ch = &world.characters[0];

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_GRAVE)) {
            debug_console_open = !debug_console_open;
        }

        if (ch->type == CHAR_WORKER) {
            logic_worker_update(ch, &world, frame_counter);
            if (IsKeyPressed(KEY_E)) {
                int bx = ch->x / 16, by = ch->y / 16;
                logic_worker_dig_block(ch, &world, bx, by);
            }
        }

        logic_update(&world, frame_counter);
        logic_bomb_update_all(&world, frame_counter);
        logic_arrow_update_all(&world, frame_counter);
        debug_console_update(&world);

        BeginDrawing();
        ClearBackground(RAYWHITE);
        draw_background(&world);
        draw_blocks(&world);
        draw_dropped_items(&world);
        draw_warrior_all(&world, frame_counter, world.local_player_id);
        draw_archer_all(&world, frame_counter, world.local_player_id);
        draw_worker_all(&world, frame_counter, world.local_player_id);
        draw_bomb_all(&world, frame_counter);
        draw_arrow_all(&world, frame_counter);
        draw_ui(&world);

        draw_debug_vectors(&world);
        draw_debug_console();

        EndDrawing();

        frame_counter++;
    }

    draw_warrior_unload();
    draw_archer_unload();
    draw_worker_unload();
    draw_bomb_unload();
    draw_arrow_unload();
    CloseWindow();
    return 0;
}
