// src/draw/draw_ui.c
#include "raylib.h"
#include "common_game.h"
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <math.h>

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

    // Счёт по центру сверху
    int screen_width = GetScreenWidth();
    snprintf(buf, sizeof(buf), "BLUE: %d  |  RED: %d", 
        world->params.blue_score, world->params.red_score);
    int text_width = MeasureTextEx(ui_font, buf, 24, 1).x;
    DrawTextEx(ui_font, buf, (Vector2){(screen_width - text_width) / 2, 20}, 24, 1, BLACK);
    
    // Индикатор прогресса захвата флага (если флаг несётся)
    if (world->flag_blue_carried || world->flag_red_carried) {
        Team carrying_team = world->flag_blue_carried ? TEAM_BLUE : TEAM_RED;
        
        // Находим игрока с флагом
        Character* carrier = NULL;
        for (int i = 0; i < world->char_count; i++) {
            if (world->characters[i].is_holding_flag) {
                carrier = &world->characters[i];
                break;
            }
        }
        
        if (carrier) {
            // Вычисляем расстояние до трона своей команды
            Vector2 throne_pos = (carrying_team == TEAM_BLUE) ? world->throne_blue : world->throne_red;
            float dx = fabsf(carrier->x - throne_pos.x);
            float dy = fabsf(carrier->y - throne_pos.y);
            float distance = sqrtf(dx * dx + dy * dy);
            
            // Максимальное расстояние для расчёта прогресса (примерно диагональ карты)
            float max_distance = world->params.width_blocks * 16.0f * 0.8f;
            float progress = 1.0f - (distance / max_distance);
            if (progress < 0.0f) progress = 0.0f;
            if (progress > 1.0f) progress = 1.0f;
            
            // Рисуем полоску прогресса
            int bar_width = 200;
            int bar_height = 20;
            int bar_x = (screen_width - bar_width) / 2;
            int bar_y = 60;
            
            // Фон полоски
            DrawRectangle(bar_x, bar_y, bar_width, bar_height, DARKGRAY);
            
            // Заполнение прогресса
            Color progress_color = (carrying_team == TEAM_BLUE) ? BLUE : RED;
            DrawRectangle(bar_x + 2, bar_y + 2, (bar_width - 4) * progress, bar_height - 4, progress_color);
            
            // Рамка
            DrawRectangleLines(bar_x, bar_y, bar_width, bar_height, WHITE);
            
            // Текст
            snprintf(buf, sizeof(buf), "FLAG CAPTURE: %.0f%%", progress * 100);
            text_width = MeasureTextEx(ui_font, buf, 16, 1).x;
            DrawTextEx(ui_font, buf, (Vector2){(screen_width - text_width) / 2, bar_y + 2}, 16, 1, WHITE);
        }
    }
    
    // Визуализация анимации флага (для отладки можно показывать силу ветра)
    // Можно добавить визуальный индикатор силы ветра в углу экрана
    char wind_buf[64];
    snprintf(wind_buf, sizeof(wind_buf), "Wind: %.2f", world->flag_anim.wind_strength);
    DrawTextEx(ui_font, wind_buf, (Vector2){20, 115}, 14, 1, LIGHTGRAY);
}
