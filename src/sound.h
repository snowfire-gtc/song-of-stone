// sound.h
#ifndef SOUND_H
#define SOUND_H

#include "../../common_game.h"


// Перечисление всех звуковых событий (должно соответствовать твоему списку!)
typedef enum {
    // Удары и атаки
    SOUND_SWORD_HIT_WOOD,
    SOUND_SWORD_HIT_STONE,
    SOUND_SWORD_HIT_GOLD,
    SOUND_SWORD_HIT_DIRT,

    // Стрелы
    SOUND_ARROW_HARVEST_WOOD,
    SOUND_ARROW_HARVEST_LEAFS,
    SOUND_ARROW_FLY,
    SOUND_ARROW_HIT_WOOD,
    SOUND_ARROW_HIT_DIRT,
    SOUND_ARROW_HIT_STONE,
    SOUND_ARROW_HIT_GOLD,
    SOUND_ARROW_HIT_CHARACTER,

    // Бомбы
    SOUND_BOMB_THROW,
    SOUND_BOMB_FUSE_TICK,
    SOUND_BOMB_EXPLODE,

    // Рабочие действия
    SOUND_DIG_GOLD,
    SOUND_DIG_STONE,
    SOUND_DIG_WOOD,
    SOUND_DIG_GRASS,
    SOUND_BUILD,
    SOUND_BUILD_DOOR,
    SOUND_PLANT_TREE,

    // Персонажи
    SOUND_PLAYER_HURT,
    SOUND_PLAYER_FALL,
    SOUND_PLAYER_DEATH,

    // Флаг
    SOUND_FLAG_CAPTURE,
    SOUND_FLAG_RETURN,
    SOUND_FLAG_SCORE,

    // Другое
    SOUND_DOOR_OPEN,
    SOUND_BLOCK_BREAK,

    // Музыкальные состояния (не звуки, но для управления)
    MUSIC_PEACEFUL,
    MUSIC_BATTLE,

    SOUND_MAX_COUNT
} SoundId;



// Инициализация всех звуков и музыки
void sound_init(void);

// Воспроизведение звука
void sound_play(SoundId id);

// Воспроизведение 3D-звука (с позицией)
void sound_play_at(SoundId id, float x, float y);

// Обновление музыки (мирная ↔ боевая)
void sound_update_music_intensity(const WorldState* world);

// Освобождение ресурсов
void sound_unload(void);
