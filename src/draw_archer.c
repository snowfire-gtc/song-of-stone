// draw_archer.c
#include "draw_archer.h"
#include "raylib.h"
#include <string.h>
#include <math.h>

// Анимации лучника
typedef struct {
    Texture2D texture;
    int frame_count;
    int frame_width;
    int frame_height;
} ArcherAnimation;

static ArcherAnimation anims[ANIM_IDLE_FUNNY + 1] = {0};
static bool textures_loaded = false;

// Загрузка анимации
static ArcherAnimation load_anim(const char* path, int frames) {
    ArcherAnimation a = {0};
    a.texture = LoadTexture(path);
    a.frame_count = frames;
    if (a.texture.id != 0) {
        a.frame_width = a.texture.width / frames;
        a.frame_height = a.texture.height;
    }
    return a;
}

// Инициализация
void draw_archer_init(void) {
    anims[ANIM_IDLE]        = load_anim("data/textures/archer/idle.png", 4);
    anims[ANIM_WALK]        = load_anim("data/textures/archer/walk.png", 6);
    anims[ANIM_JUMP]        = load_anim("data/textures/archer/jump.png", 2);
    anims[ANIM_ATTACK]      = load_anim("data/textures/archer/shoot.png", 4);
    anims[ANIM_DIG]         = load_anim("data/textures/archer/harvest.png", 3);
    anims[ANIM_SWIM]        = load_anim("data/textures/archer/swim.png", 4);
    anims[ANIM_CLIMB]       = load_anim("data/textures/archer/climb.png", 2);
    anims[ANIM_IDLE_FUNNY]  = load_anim("data/textures/archer/idle_funny.png", 3);

    textures_loaded = true;
}

// Получение кадра
static Rectangle get_src_rect(const ArcherAnimation* a, int frame) {
    frame = frame % a->frame_count;
    return (Rectangle){frame * a->frame_width, 0, a->frame_width, a->frame_height};
}

// Отрисовка одного лучника
void draw_archer_single(const Character* archer, int frame_counter, bool is_local_player) {
    if (archer->hp <= 0) return;

    // Мерцание при неуязвимости
    if (archer->is_invulnerable && (frame_counter % 20 >= 10)) return;

    // Выбор анимации
    AnimationState state = archer->anim_state;
    if (archer->is_aiming) state = ANIM_ATTACK;
    if (archer->is_climbing) state = ANIM_CLIMB;

    if (state < 0 || state > ANIM_IDLE_FUNNY) state = ANIM_IDLE;
    const ArcherAnimation* anim = &anims[state];

    if (anim->texture.id == 0 && !textures_loaded) {
        // === FALLBACK: ГЕОМЕТРИЯ ===
        Color body = is_local_player ? GREEN : 
                    (archer->team == TEAM_BLUE ? ColorAlpha(BLUE, 0.7f) : ColorAlpha(RED, 0.7f));

        // Тело
        DrawRectangle(archer->x - 4, archer->y - 32, 8, 32, body);

        // Лук (если целится)
        if (archer->is_aiming) {
            Color bow = BROWN;
            DrawLine(archer->x + (archer->team == TEAM_BLUE ? 0 : -6), archer->y - 16,
                     archer->x + (archer->team == TEAM_BLUE ? 6 : 0), archer->y - 20, bow);
            // Тетива (масштабируется от силы)
            float power = fminf(archer->aim_time, 1.0f);
            DrawLine(archer->x, archer->y - 18,
                     archer->x - (archer->team == TEAM_BLUE ? -1 : 1) * (int)(8 * power), archer->y - 18, YELLOW);
        }

        goto draw_overlays; // переходим к элементам поверх
    }

    // === ОТРИСОВКА СПРАЙТА ===
    int speed = (state == ANIM_WALK) ? 6 : 10;
    int frame = (archer->frame_counter / speed) % anim->frame_count;
    Rectangle src = get_src_rect(anim, frame);
    Rectangle dst = {
        .x = archer->x - anim->frame_width / 2,
        .y = archer->y - anim->frame_height,
        .width = anim->frame_width,
        .height = anim->frame_height
    };
    Color tint = is_local_player ? WHITE : 
                (archer->team == TEAM_BLUE ? ColorAlpha(BLUE, 0.7f) : ColorAlpha(RED, 0.7f));

    DrawTexturePro(anim->texture, src, dst, (Vector2){0,0}, 0, tint);

draw_overlays:
    // === ЭЛЕМЕНТЫ ПОВЕРХ ===

    // Индикатор силы выстрела
    if (archer->is_aiming) {
        float power = fminf(archer->aim_time, 1.0f);
        Color bar_color = (power < 0.3f) ? RED : (power < 0.7f ? ORANGE : GREEN);
        DrawRectangle(archer->x - 20, archer->y - 40, (int)(40 * power), 4, bar_color);
        DrawRectangleLines(archer->x - 20, archer->y - 40, 40, 4, BLACK);
    }

    // Флаг (если несёт вражеский)
    if (archer->is_holding_flag) {
        Color flag = (archer->team == TEAM_BLUE) ? RED : BLUE;
        DrawRectangle(archer->x + 6, archer->y - 22, 2, 10, flag);
        DrawTriangle(
            (Vector2){archer->x + 6, archer->y - 22},
            (Vector2){archer->x + 12, archer->y - 20},
            (Vector2){archer->x + 6, archer->y - 18},
            flag
        );
    }

    // Отладка: стрелы
    #ifdef DEBUG_MODE
    char buf[32];
    snprintf(buf, sizeof(buf), "A: %d", archer->arrows);
    DrawText(buf, archer->x - 10, archer->y - 45, 10, BLACK);
    #endif
}

// Отрисовка всех лучников
void draw_archer_all(const WorldState* world, int frame_counter, int local_player_id) {
    for (int i = 0; i < world->char_count; i++) {
        const Character* ch = &world->characters[i];
        if (ch->type != CHAR_ARCHER) continue;
        bool is_local = (ch->player_id == local_player_id);
        draw_archer_single(ch, frame_counter, is_local);
    }
}

// Освобождение
void draw_archer_unload(void) {
    for (int i = 0; i <= ANIM_IDLE_FUNNY; i++) {
        if (anims[i].texture.id != 0) {
            UnloadTexture(anims[i].texture);
        }
    }
}
