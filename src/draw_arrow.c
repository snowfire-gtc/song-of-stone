// draw_arrow.c
#include "draw_arrow.h"
#include "raylib.h"
#include <math.h>

static Texture2D arrow_texture = {0};
static bool texture_loaded = false;

// Инициализация
void draw_arrow_init(void) {
    arrow_texture = LoadTexture("data/textures/arrow.png"); // 16x4 пикс
    texture_loaded = (arrow_texture.id != 0);
    if (!texture_loaded) {
        TraceLog(LOG_INFO, "Arrow texture not found. Using geometric fallback.");
    }
}

// Отрисовка одной стрелы
void draw_arrow_single(const Arrow* arrow) {
    if (arrow->hit) return;

    if (texture_loaded) {
        // Поворот по направлению полёта
        float angle = atan2f(arrow->vy, arrow->vx) * (180.0f / M_PI);
        Rectangle src = {0, 0, 16, 4};
        Rectangle dst = {arrow->x - 8, arrow->y - 2, 16, 4};
        Vector2 origin = {0, 2}; // от хвоста

        DrawTexturePro(arrow_texture, src, dst, origin, angle, BLACK);
    } else {
        // FALLBACK: линия + наконечник
        Vector2 start = {arrow->x, arrow->y};
        Vector2 end = {arrow->x + arrow->vx * 0.1f, arrow->y + arrow->vy * 0.1f};
        DrawLineEx(start, end, 2, BLACK);

        // Наконечник (треугольник)
        float angle = atan2f(arrow->vy, arrow->vx);
        Vector2 tip = end;
        Vector2 left = {
            tip.x + 6 * cosf(angle + 2.5f),
            tip.y + 6 * sinf(angle + 2.5f)
        };
        Vector2 right = {
            tip.x + 6 * cosf(angle - 2.5f),
            tip.y + 6 * sinf(angle - 2.5f)
        };
        DrawTriangle(tip, left, right, BLACK);
    }
}

// Отрисовка всех стрел
void draw_arrow_all(const WorldState* world) {
    for (int i = 0; i < world->arrow_count; i++) {
        draw_arrow_single(&world->arrows[i]);
    }
}

// Освобождение
void draw_arrow_unload(void) {
    if (arrow_texture.id != 0) {
        UnloadTexture(arrow_texture);
    }
}
