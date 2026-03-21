// draw_worker.c
#include "draw_worker.h"
#include "raylib.h"

typedef struct {
    Texture2D texture;
    int frames;
    int w, h;
} WorkerAnim;

static WorkerAnim anims[ANIM_IDLE_FUNNY + 1] = {0};
static bool loaded = false;

static WorkerAnim load(const char* f, int fr) {
    WorkerAnim a = {0};
    a.texture = LoadTexture(f);
    a.frames = fr;
    if (a.texture.id) {
        a.w = a.texture.width / fr;
        a.h = a.texture.height;
    }
    return a;
}

void draw_worker_init(void) {
    anims[ANIM_IDLE]        = load("data/textures/worker/idle.png", 4);
    anims[ANIM_WALK]        = load("data/textures/worker/walk.png", 6);
    anims[ANIM_DIG]         = load("data/textures/worker/dig.png", 4);
    anims[ANIM_JUMP]        = load("data/textures/worker/jump.png", 2);
    anims[ANIM_SWIM]        = load("data/textures/worker/swim.png", 4);
    anims[ANIM_IDLE_FUNNY]  = load("data/textures/worker/idle_funny.png", 3);
    loaded = true;
}

static Rectangle src_rect(const WorkerAnim* a, int frame) {
    return (Rectangle){frame * a->w, 0, a->w, a->h};
}

void draw_worker_single(const Character* w, int fc, bool local) {
    if (w->hp <= 0) return;
    if (w->is_invulnerable && (fc % 20 >= 10)) return;

    AnimationState s = w->anim_state;
    if (s < 0 || s > ANIM_IDLE_FUNNY) s = ANIM_IDLE;
    const WorkerAnim* a = &anims[s];

    if (a->texture.id == 0 && !loaded) {
        // Fallback
        Color c = local ? BROWN : (w->team == TEAM_BLUE ? ColorAlpha(BLUE, 0.6f) : ColorAlpha(RED, 0.6f));
        DrawRectangle(w->x - 4, w->y - 32, 8, 32, c);
        goto overlay;
    }

    int speed = (s == ANIM_WALK) ? 6 : 10;
    int frame = (w->frame_counter / speed) % a->frames;
    Rectangle src = src_rect(a, frame);
    Rectangle dst = {w->x - a->w/2, w->y - a->h, a->w, a->h};
    Color tint = local ? WHITE : (w->team == TEAM_BLUE ? ColorAlpha(BLUE, 0.6f) : ColorAlpha(RED, 0.6f));
    DrawTexturePro(a->texture, src, dst, (Vector2){0,0}, 0, tint);

overlay:
    if (w->is_holding_flag) {
        Color flag = (w->team == TEAM_BLUE) ? RED : BLUE;
        DrawRectangle(w->x + 6, w->y - 22, 2, 10, flag);
        DrawTriangle((Vector2){w->x+6, w->y-22}, (Vector2){w->x+12, w->y-20}, (Vector2){w->x+6, w->y-18}, flag);
    }
}

void draw_worker_all(const WorldState* w, int fc, int lid) {
    for (int i = 0; i < w->char_count; i++) {
        if (w->characters[i].type == CHAR_WORKER) {
            draw_worker_single(&w->characters[i], fc, w->characters[i].player_id == lid);
        }
    }
}

void draw_worker_unload(void) {
    for (int i = 0; i <= ANIM_IDLE_FUNNY; i++) {
        if (anims[i].texture.id) UnloadTexture(anims[i].texture);
    }
}
