// net_protocol.h - Улучшенный протокол с надежностью и безопасностью
#ifndef NET_PROTOCOL_H
#define NET_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "common_game.h"

#define NET_PORT 26000
#define MAX_PLAYERS 128
#define SNAPSHOT_RATE 30       // снапшоты в секунду
#define INPUT_RATE 60          // ввод в секунду
#define BUFFER_SIZE 4096
#define MAX_PACKET_SIZE 1400   // MTU для UDP

// --- Типы пакетов ---
typedef enum {
    PKT_NONE = 0,
    PKT_HELLO,          // клиент → сервер: "я новый игрок"
    PKT_WELCOME,        // сервер → клиент: добро пожаловать (ID)
    PKT_INPUT,          // клиент → сервер: управление
    PKT_SNAPSHOT,       // сервер → клиент: состояние мира
    PKT_DELTA,          // сервер → клиент: дельта-обновление
    PKT_BLOCK_CHANGE,   // клиент → сервер: изменение блока
    PKT_CHAT,           // все ↔ все: чат
    PKT_DISCONNECT,     // все: отключение
    PKT_ACK,            // подтверждение получения
    PKT_HEARTBEAT       // проверка соединения
} PacketType;

// --- Заголовок пакета с надежностью ---
typedef struct {
    uint8_t type;           // Тип пакета
    uint8_t flags;          // Флаги (бит 0: требует ACK, бит 1: сжатие)
    uint16_t size;          // Размер данных
    uint32_t sequence;      // Порядковый номер
    uint32_t ack;           // Подтвержденный номер (если есть)
    uint32_t timestamp;     // Время отправки (мс)
    uint32_t checksum;      // Контрольная сумма
} PacketHeader;

#define PACKET_FLAG_RELIABLE  0x01
#define PACKET_FLAG_COMPRESSED 0x02

// --- Ввод игрока (сжатый) ---
typedef struct {
    uint8_t move_left   : 1;
    uint8_t move_right  : 1;
    uint8_t jump        : 1;
    uint8_t attack      : 1;
    uint8_t aim         : 1;
    uint8_t dig         : 1;
    uint8_t shield      : 1;
    uint8_t build       : 1;  // строительство
    uint8_t block_type;       // тип блока для строительства
    uint8_t extra;            // для будущего
    int16_t mouse_x;          // смещение мыши
    int16_t mouse_y;
} PlayerInput;

// --- Снапшот персонажа (оптимизированный) ---
typedef struct {
    uint16_t id;
    int16_t x, y;             // позиция (смещенная)
    int8_t vx, vy;            // скорость / 4
    uint8_t hp;               // 0–6
    uint8_t max_hp;
    uint8_t anim_state;
    uint8_t team             : 2;
    uint8_t type             : 2;
    uint8_t is_shield_active : 1;
    uint8_t is_aiming        : 1;
    uint8_t is_holding_flag  : 1;
    uint8_t is_climbing      : 1;
    uint8_t is_alive         : 1;
    uint8_t padding;          // выравнивание
    // Инвентарь (дельта)
    uint16_t coins;
    uint16_t wood;
    uint16_t stone;
    uint16_t arrows;
    uint16_t bombs;
} SnapshotChar;

// --- Дельта-обновление позиции ---
typedef struct {
    uint16_t id;
    int16_t dx, dy;           // смещение относительно предыдущего
    int8_t dvx, dvy;          // изменение скорости
} DeltaPos;

// --- Изменение блока ---
typedef struct {
    int16_t x, y;
    uint8_t block_id;
    uint8_t action;           // 0: place, 1: break
    uint32_t timestamp;
} BlockChange;

// --- Полный снапшот мира ---
typedef struct {
    uint32_t tick;
    uint32_t game_time;
    uint8_t char_count;
    uint8_t block_changes_count;
    uint8_t score_blue;
    uint8_t score_red;
    uint8_t flag_progress;    // 0-100%
    SnapshotChar chars[MAX_PLAYERS];
    BlockChange block_changes[32];  // последние изменения блоков
} WorldSnapshot;

// --- Пакет ---
typedef struct {
    PacketHeader header;
    union {
        PlayerInput input;
        WorldSnapshot snapshot;
        DeltaPos delta;
        BlockChange block_change;
        struct {
            uint16_t player_id;
            char name[MAX_NAME_LEN];
        } hello;
        struct {
            uint16_t assigned_id;
            uint8_t team;
        } welcome;
        struct {
            char text[MAX_CHAT_LEN];
            uint16_t player_id;
        } chat;
        uint8_t raw[BUFFER_SIZE - sizeof(PacketHeader)];
    } data;
} NetworkPacket;

// --- Функции утилит ---

// Расчет контрольной суммы (CRC32 или простая)
uint32_t calculate_checksum(const void* data, size_t len);
bool verify_packet(const NetworkPacket* pkt);

// Сериализация/десериализация
size_t serialize_packet(const NetworkPacket* pkt, uint8_t* buffer, size_t buf_size);
bool deserialize_packet(const uint8_t* buffer, size_t len, NetworkPacket* pkt);

// Дельта-кодирование
void encode_delta(const SnapshotChar* current, const SnapshotChar* previous, DeltaPos* delta);
void decode_delta(const DeltaPos* delta, SnapshotChar* previous, SnapshotChar* result);

// Сжатие (простое RLE для блоков)
size_t compress_block_changes(const BlockChange* changes, uint8_t count, uint8_t* output);
size_t decompress_block_changes(const uint8_t* input, size_t len, BlockChange* output, size_t max_count);

#endif // NET_PROTOCOL_H
