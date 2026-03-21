// draw_bomb.c
#include "draw_bomb.h"
#include "raylib.h"
#include <stdlib.h>

// Глобальные текстуры
static Texture2D bomb_texture = {0};
static Texture2D explosion_frames[4] = {0};
static bool textures_loaded = false;

// Инициализация
void draw_bomb_init(void) {
    bomb_texture = LoadTexture("data/textures/bomb.png");

    // Загрузка кадров взрыва (опционально)
    explosion_frames[0] = LoadTexture("data/textures/explosion_01.png");
    explosion_frames[1] = LoadTexture("data/textures/explosion_02.png");
    explosion_frames[2] = LoadTexture("data/textures/explosion_03.png");
    explosion_frames[3] = LoadTexture("data/textures/explosion_04.png");

    textures_loaded = true;

    // Проверка загрузки
    if (bomb_texture.id == 0) {
        TraceLog(LOG_WARNING, "Bomb texture not found! Using fallback circle.");
    }
}

// Отрисовка одной бомбы
void draw_bomb_single(const Bomb* bomb, int frame_counter) {
    if (bomb->exploded) {
        //// Анимация взрыва (последние 0.2 сек)
        //// Упрощённо: рисуем большой круг
        //Color flash = (frame_counter % 6 < 3) ? ORANGE : YELLOW;
        //DrawCircle(bomb->x, bomb->y, 24, flash);
        //DrawCircle(bomb->x, bomb->y, 16, RED);

        // Анимация с картинками
        int frame = (int)(-bomb->timer * 20) % 4; // 20 fps
        DrawTexture(explosion_frames[frame], bomb->x - 16, bomb->y - 16, WHITE);
        return;
    }

    // Мигание, если остаётся <1 сек до взрыва
    bool flash = (bomb->timer < 1.0f) && ((int)(bomb->timer * 20) % 4 < 2);

    // Основная отрисовка
    if (textures_loaded && bomb_texture.id != 0) {
        Rectangle src = {0, 0, 16, 16};
        Rectangle dst = {bomb->x - 8, bomb->y - 8, 16, 16}; // центрирование
        Vector2 origin = {8, 8};

        // Поворот по траектории (если летит)
        float rotation = 0.0f;
        if (bomb->state == BOMB_STATE_FLYING) {
            rotation = atan2f(bomb->vy, bomb->vx) * (180.0f / M_PI);
        }

        Color tint = flash ? YELLOW : WHITE;
        DrawTexturePro(bomb_texture, src, dst, origin, rotation, tint);
    } else {
        // Fallback: круг
        Color color = flash ? YELLOW : BLACK;
        DrawCircle(bomb->x, bomb->y, 6, color);
        DrawCircleLines(bomb->x, bomb->y, 6, GRAY);
    }

    // Отладка: таймер (только в debug-режиме)
    #ifdef DEBUG_MODE
    char timer_str[16];
    snprintf(timer_str, sizeof(timer_str), "%.1fs", bomb->timer);
    DrawText(timer_str, bomb->x - 20, bomb->y - 20, 10, RED);
    #endif
}

// Отрисовка всех бомб
void draw_bomb_all(const WorldState* world, int frame_counter) {
    for (int i = 0; i < world->bomb_count; i++) {
        const Bomb* bomb = &world->bombs[i];
        if (bomb->exploded && bomb->timer > -0.5f) {
            // Показываем взрыв ещё 0.5 сек
            draw_bomb_single(bomb, GetFrameCounter()); // см. ниже
        } else if (!bomb->exploded) {
            draw_bomb_single(bomb, GetFrameCounter());
        }
    }
}

// Освобождение
void draw_bomb_unload(void) {
    if (bomb_texture.id != 0) UnloadTexture(bomb_texture);
    for (int i = 0; i < 4; i++) {
        if (explosion_frames[i].id != 0) UnloadTexture(explosion_frames[i]);
    }
}
