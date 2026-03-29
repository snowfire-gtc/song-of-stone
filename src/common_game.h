// common_game.h
#ifndef COMMON_GAME_H
#define COMMON_GAME_H

#include <stdbool.h>
#include <stdint.h>
#include "raylib.h"

// ========= ENUMS с префиксами =========

typedef enum {
    CHAR_WORKER,
    CHAR_WARRIOR,
    CHAR_ARCHER
} CharacterType;

typedef enum {
    BLOCK_AIR,
    BLOCK_WATER,
    BLOCK_DIRT,
    BLOCK_LAVA,
    BLOCK_STONE,
    BLOCK_WOOD,
    BLOCK_LEAFS,
    BLOCK_GOLD,
    BLOCK_SPIKES,
    BLOCK_BRIDGE,
    BLOCK_LADDER,
    BLOCK_DOOR,
    BLOCK_GRASS,  // декоративный слой на dirt
    BLOCK_BOMB    // бомба как блок (1x1)
} BlockType;

typedef enum {
    TEAM_BLUE,
    TEAM_RED,
    TEAM_NONE
} Team;

typedef enum {
    ITEM_COINS,
    ITEM_WOOD,
    ITEM_STONE,
    ITEM_ARROWS,
    ITEM_BOMBS,
    ITEM_FLAG,
    ITEM_ARROW_PROJECTILE  // ЛЕТЯЩАЯ СТРЕЛА
} ItemType;

typedef enum {
    ANIM_IDLE,
    ANIM_WALK,
    ANIM_JUMP,
    ANIM_ATTACK,
    ANIM_DIG,
    ANIM_SHIELD,
    ANIM_SWIM,
    ANIM_CLIMB,
    ANIM_HURT,
    ANIM_IDLE_FUNNY  // "смешная анимация"
} AnimationState;

typedef enum {
    BOMB_STATE_IDLE,
    BOMB_STATE_FLYING,
    BOMB_STATE_PLANTED,
    BOMB_STATE_EXPLODING
} BombState;

// Новая структура для стрелы в полёте
typedef struct {
    int x, y;               // позиция в пикселях
    float vx, vy;           // скорость
    Team owner_team;        // чья стрела
    bool hit;               // столкнулась?
    float rotation;         // угол поворота (радианы)
} Arrow;

// ========= КОНСТАНТЫ =========

#define MAX_PLAYERS 128
#define MAX_NAME_LEN 16
#define WORLD_MAX_WIDTH 1024
#define WORLD_MAX_HEIGHT 256
#define PLAYER_MAX_HP 6
#define PLAYER_OXYGEN_MAX 30
#define FLAG_RETURN_RADIUS 2  // блоков от трона
#define BOMB_EXPLOSION_RADIUS 2
#define DAMAGE_FALL_THRESHOLD 10  // блоков
#define DAMAGE_FALL_PER_STEP 4
#define MAX_FALL_DEATH_HEIGHT 34
// В common_game.h

#define WORKER_DIG_SPEED_DIRT 0.2f    // секунд на удар по почве
#define WORKER_DIG_SPEED_STONE 0.5f   // по камню
#define WORKER_DIG_SPEED_GOLD 0.3f    // по золоту

#define WORKER_BUILD_COST_SPIKES 20
#define WORKER_BUILD_COST_BRIDGE 20
#define WORKER_BUILD_COST_LADDER 10
#define WORKER_BUILD_COST_DOOR 20

// ========= СТРУКТУРЫ =========

typedef struct {
    int x, y;
    BlockType type;
    bool has_grass;       // только для BLOCK_DIRT
    uint8_t grass_variant; // 0–3
} Block;

typedef struct {
    int x, y;             // в пикселях
    float vx, vy;
    CharacterType type;
    Team team;
    int hp;
    bool is_shield_active;
    bool is_charging;
    float charge_time;
    AnimationState anim_state;
    int frame_counter;
    bool is_invulnerable; // после возрождения
    float invuln_timer;
    bool is_holding_flag;
    char name[MAX_NAME_LEN];
    int player_id;

    // Инвентарь
    int coins;
    int wood;
    int stone;
    int arrows;
    int bombs;

    // Кислород
    int oxygen;

    // Визуал
    int head_style;
    int costume_style;

    // Только для лучника
    bool is_aiming;          // натягивает тетиву
    float aim_time;          // длительность натяжки (0.0–1.0+ сек)
    bool is_climbing;        // лезет по дереву/стене
    int climbing_block_x;    // позиция блока, по которому лезет
} Character;

typedef struct {
    int x, y;               // позиция в пикселях
    float vx, vy;           // скорость
    BombState state;
    float timer;            // время до взрыва (в секундах)
    int owner_id;           // кто бросил
    Team owner_team;
    bool exploded;
} Bomb;

typedef struct {
    int x, y;
    ItemType type;
    int amount;
    Team team; // для флага — кому принадлежит
    float vx, vy;
    bool is_picked_up;
} DroppedItem;

typedef struct {
    int width_blocks;
    int height_blocks;
    float gravity;
    float max_speed;
    float friction;
    int bomb_fuse_time_seconds;
    float respawn_time_per_block; // sec/block (max 120 sec)
    float dropped_coin_ratio;     // 0.5 = 50%
    float flag_capture_share;     // доля для команды (0.2 = 20%)

    struct {
        int spikes_wood;
        int bridge_wood;
        int ladder_wood;
        int door_wood;
        int arrows_per_10_coins;
        int bombs_per_20_coins;
    } build_costs;

    // Счёт
    int blue_score;
    int red_score;
} WorldParams;

typedef struct {
    Block blocks[WORLD_MAX_HEIGHT][WORLD_MAX_WIDTH];
    Character characters[MAX_PLAYERS];
    DroppedItem items[MAX_PLAYERS * 4];
    int char_count;
    int item_count;
    Vector2 throne_blue;
    Vector2 throne_red;
    Vector2 flag_blue_pos;
    Vector2 flag_red_pos;
    bool flag_blue_carried;
    bool flag_red_carried;
    int flag_carrier_id; // если != -1

    WorldParams params;
    bool game_over;
    Team winner;
    bool is_multiplayer;
    int local_player_id;

    Bomb bombs[MAX_PLAYERS * 2];  // максимум 2 бомбы на игрока
    Arrow arrows[MAX_PLAYERS * 10];  // до 10 стрел на игрока
    int bomb_count;
    int arrow_count;
} WorldState;

// common_game.h


#endif // COMMON_GAME_H
