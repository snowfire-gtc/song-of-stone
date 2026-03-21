// sound.c
#include "sound.h"
#include "raylib.h"
#include <math.h>

#define MUSIC_TRANSITION_TIME 3.0f // секунды на плавный переход

static Sound sounds[SOUND_MAX_COUNT] = {0};
static Music music_peaceful = {0};
static Music music_battle = {0};
static bool is_battle_mode = false;
static float music_blend = 0.0f; // 0 = peaceful, 1 = battle

// Вспомогательная функция: загрузка звука
static Sound load_sound_safe(const char* path) {
    Sound s = LoadSound(path);
    if (s.stream.buffer == NULL) {
        TraceLog(LOG_WARNING, "Sound not found: %s", path);
    }
    return s;
}

// Инициализация
void sound_init(void) {
    // Звуки меча
    sounds[SOUND_SWORD_HIT_WOOD]   = load_sound_safe("data/sounds/sword/hit_wood.ogg");
    sounds[SOUND_SWORD_HIT_STONE]  = load_sound_safe("data/sounds/sword/hit_stone.ogg");
    sounds[SOUND_SWORD_HIT_GOLD]   = load_sound_safe("data/sounds/sword/hit_gold.ogg");
    sounds[SOUND_SWORD_HIT_DIRT]   = load_sound_safe("data/sounds/sword/hit_dirt.ogg");

    // Стрелы
    sounds[SOUND_ARROW_HARVEST_WOOD] = load_sound_safe("data/sounds/arrow/harvest_wood.ogg");
    sounds[SOUND_ARROW_HARVEST_LEAFS] = load_sound_safe("data/sounds/arrow/harvest_leafs.ogg");
    sounds[SOUND_ARROW_FLY]           = load_sound_safe("data/sounds/arrow/fly.ogg");
    sounds[SOUND_ARROW_HIT_WOOD]      = load_sound_safe("data/sounds/arrow/hit_wood.ogg");
    sounds[SOUND_ARROW_HIT_DIRT]      = load_sound_safe("data/sounds/arrow/hit_dirt.ogg");
    sounds[SOUND_ARROW_HIT_STONE]     = load_sound_safe("data/sounds/arrow/hit_stone.ogg");
    sounds[SOUND_ARROW_HIT_GOLD]      = load_sound_safe("data/sounds/arrow/hit_gold.ogg");
    sounds[SOUND_ARROW_HIT_CHARACTER] = load_sound_safe("data/sounds/arrow/hit_character.ogg");

    // Бомбы
    sounds[SOUND_BOMB_THROW]     = load_sound_safe("data/sounds/bomb/throw.ogg");
    sounds[SOUND_BOMB_FUSE_TICK] = load_sound_safe("data/sounds/bomb/fuse_tick.ogg");
    sounds[SOUND_BOMB_EXPLODE]   = load_sound_safe("data/sounds/bomb/explode.ogg");

    // Рабочие
    sounds[SOUND_DIG_GOLD]       = load_sound_safe("data/sounds/worker/dig_gold.ogg");
    sounds[SOUND_DIG_STONE]      = load_sound_safe("data/sounds/worker/dig_stone.ogg");
    sounds[SOUND_DIG_WOOD]       = load_sound_safe("data/sounds/worker/dig_wood.ogg");
    sounds[SOUND_DIG_GRASS]      = load_sound_safe("data/sounds/worker/dig_grass.ogg");
    sounds[SOUND_BUILD]          = load_sound_safe("data/sounds/worker/build.ogg");
    sounds[SOUND_BUILD_DOOR]     = load_sound_safe("data/sounds/worker/build_door.ogg");
    sounds[SOUND_PLANT_TREE]     = load_sound_safe("data/sounds/worker/plant_tree.ogg");

    // Персонажи
    sounds[SOUND_PLAYER_HURT]    = load_sound_safe("data/sounds/player/hurt.ogg");
    sounds[SOUND_PLAYER_FALL]    = load_sound_safe("data/sounds/player/fall.ogg");
    sounds[SOUND_PLAYER_DEATH]   = load_sound_safe("data/sounds/player/death.ogg");

    // Флаг
    sounds[SOUND_FLAG_CAPTURE]   = load_sound_safe("data/sounds/flag/capture.ogg");
    sounds[SOUND_FLAG_RETURN]    = load_sound_safe("data/sounds/flag/return.ogg");
    sounds[SOUND_FLAG_SCORE]     = load_sound_safe("data/sounds/flag/score.ogg");

    // Другое
    sounds[SOUND_DOOR_OPEN]      = load_sound_safe("data/sounds/other/door_open.ogg");
    sounds[SOUND_BLOCK_BREAK]    = load_sound_safe("data/sounds/other/block_break.ogg");

    // Музыка
    music_peaceful = LoadMusicStream("data/sounds/music/peaceful.ogg");
    music_battle   = LoadMusicStream("data/sounds/music/battle.ogg");

    PlayMusicStream(music_peaceful);
    PlayMusicStream(music_battle);
    SetMusicVolume(music_battle, 0.0f); // начинаем с тишины
}

// Воспроизведение звука
void sound_play(SoundId id) {
    if (id >= SOUND_MAX_COUNT) return;
    if (sounds[id].stream.buffer == NULL) return;
    PlaySound(sounds[id]);
}

// Пространственный звук (моно → стерео на основе позиции)
void sound_play_at(SoundId id, float x, float y) {
    if (id >= SOUND_MAX_COUNT) return;
    Sound* s = &sounds[id];
    if (s->stream.buffer == NULL) return;

    // Простая панорама: лево/право
    float screen_center = GetScreenWidth() / 2.0f;
    float pan = (x - screen_center) / screen_center; // -1..1
    pan = fmaxf(-1.0f, fminf(1.0f, pan));

    SetSoundPan(*s, pan);
    PlaySound(*s);
}

// Обновление музыки
void sound_update_music_intensity(const WorldState* world) {
    // Пример: если рядом >2 врага → боевой режим
    bool intense = false;
    const Character* local = &world->characters[world->local_player_id];
    int enemies_near = 0;

    for (int i = 0; i < world->char_count; i++) {
        const Character* ch = &world->characters[i];
        if (ch->team != local->team && ch->hp > 0) {
            float dx = fabsf(local->x - ch->x);
            float dy = fabsf(local->y - ch->y);
            if (dx < 200 && dy < 200) { // 200 пикс = ~12 блоков
                enemies_near++;
            }
        }
    }
    intense = (enemies_near >= 2);

    // Плавный переход
    if (intense && !is_battle_mode) {
        music_blend = fminf(music_blend + GetFrameTime() / MUSIC_TRANSITION_TIME, 1.0f);
        if (music_blend >= 1.0f) is_battle_mode = true;
    } else if (!intense && is_battle_mode) {
        music_blend = fmaxf(music_blend - GetFrameTime() / MUSIC_TRANSITION_TIME, 0.0f);
        if (music_blend <= 0.0f) is_battle_mode = false;
    }

    // Установка громкости
    SetMusicVolume(music_peaceful, 1.0f - music_blend);
    SetMusicVolume(music_battle, music_blend);

    // Обновление потоков
    UpdateMusicStream(music_peaceful);
    UpdateMusicStream(music_battle);
}

// Освобождение
void sound_unload(void) {
    for (int i = 0; i < SOUND_MAX_COUNT; i++) {
        if (sounds[i].stream.buffer) {
            UnloadSound(sounds[i]);
        }
    }
    UnloadMusicStream(music_peaceful);
    UnloadMusicStream(music_battle);
}
