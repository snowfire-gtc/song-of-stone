// src/draw/draw_ui.c
#include "raylib.h"
#include "common_game.h"
#include <stdio.h>
#include <string.h>
#include <stddef.h>

extern Font ui_font; // используем шрифт из draw.c

void init_ui_font(void) {
    // Шрифт уже инициализирован в draw.c через init_ui_font_internal()
}

void draw_ui(const WorldState* world) {
    const Character* p = &world->characters[world->local_player_id];

    // Жизни: 1 heart = 2 HP
    char hearts[32] = {0};
    int heart_count = (p->hp + 1) / 2;
    for (int i = 0; i < heart_count; i++) {
        strcat(hearts, "\xE2\x99\xA5"); // U+2665 = ♥
    }
    DrawTextEx(ui_font, hearts, (Vector2){20, 20}, 24, 1, RED);

    // Ресурсы
    char buf[256];
    snprintf(buf, sizeof(buf), 
        "\xE2\x82\xAC%d  %d\xE2\x9C\x9F  %d\xE2\x9C\xA8  %d\xF0\x9F\x8F\xB9  %d\xF0\x9F\x92\xA3",
        p->coins, p->wood, p->stone, p->arrows, p->bombs
    );
    DrawTextEx(ui_font, buf, (Vector2){20, 55}, 20, 1, DARKGRAY);

    // Кислород (только под водой)
    if (p->oxygen < PLAYER_OXYGEN_MAX) {
        snprintf(buf, sizeof(buf), "\xF0\x9F\x92\xA7 %d", p->oxygen);
        DrawTextEx(ui_font, buf, (Vector2){20, 85}, 18, 1, BLUE);
    }

    // Счёт
    snprintf(buf, sizeof(buf), "%d  %d", 
        world->params.blue_score, world->params.red_score);
    DrawTextEx(ui_font, buf, (Vector2){GetScreenWidth() - 180, 20}, 20, 1, BLACK);
}
