// net_protocol.h
#ifndef NET_PROTOCOL_H
#define NET_PROTOCOL_H

#include <stdint.h>
#include "../common_game.h"

#define NET_PORT 26000
#define MAX_PLAYERS 128
#define SNAPSHOT_RATE 30       // снапшоты в секунду
#define INPUT_RATE 60          // ввод в секунду

// Типы пакетов
typedef enum {
    PKT_HELLO,          // клиент → сервер: "я новый игрок"
    PKT_INPUT,          // клиент → сервер: управление
    PKT_SNAPSHOT,       // сервер → клиент: состояние мира
    PKT_DISCONNECT      // клиент → сервер: выход
} PacketType;

// Ввод игрока (сжатый)
typedef struct {
    uint8_t move_left   : 1;
    uint8_t move_right  : 1;
    uint8_t jump        : 1;
    uint8_t attack      : 1;
    uint8_t aim         : 1;
    uint8_t dig         : 1;
    uint8_t shield      : 1;
    uint8_t unused      : 1;
    uint8_t extra;      // для будущего
} PlayerInput;

// Снапшот персонажа (минимизированный)
typedef struct {
    uint16_t x, y;              // позиция (пикс)
    int8_t vx, vy;              // скорость / 4
    uint8_t hp;                 // 0–6
    uint8_t anim_state;
    uint8_t is_shield_active : 1;
    uint8_t is_aiming        : 1;
    uint8_t is_holding_flag  : 1;
    uint8_t is_climbing      : 1;
    uint8_t team             : 2;
    uint8_t type             : 2;
} SnapshotChar;

// Полный снапшот мира
typedef struct {
    uint32_t tick;              // номер тика
    uint8_t char_count;
    SnapshotChar chars[MAX_PLAYERS];
    // Другие данные (флаги, счёт) — опущены для краткости
} WorldSnapshot;

// Пакеты
typedef struct {
    uint8_t type;
    uint16_t player_id;
    union {
        PlayerInput input;
        WorldSnapshot snapshot;
        char hello_name[MAX_NAME_LEN];
    };
} NetworkPacket;

#endif // NET_PROTOCOL_H
