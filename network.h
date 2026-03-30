#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>
#include <stdbool.h>

// Константы сети
#define SERVER_PORT 5555
#define MAX_PLAYERS 4
#define WORLD_WIDTH 64
#define WORLD_HEIGHT 32
#define TICK_RATE 60

// Типы сообщений
typedef enum {
    MSG_NONE = 0,
    MSG_CONNECT_REQ,
    MSG_CONNECT_RESP,
    MSG_GAME_STATE,
    MSG_PLAYER_INPUT,
    MSG_BLOCK_UPDATE,
    MSG_FLAG_UPDATE,
    MSG_CHAT
} MessageType;

// Типы блоков (должны совпадать с main.c)
typedef enum {
    BLOCK_AIR = 0,
    BLOCK_DIRT,
    BLOCK_STONE,
    BLOCK_WOOD,
    BLOCK_GOLD,
    BLOCK_WATER,
    BLOCK_LAVA,
    BLOCK_SAND,
    BLOCK_GRAVEL,
    BLOCK_THRONE_BLUE,
    BLOCK_THRONE_RED,
    BLOCK_FLAG_BLUE,
    BLOCK_FLAG_RED
} BlockType;

// Пакет подключения
typedef struct {
    uint8_t type;
    char username[32];
    int team; // 0: Blue, 1: Red
} PacketConnect;

// Пакет ответа на подключение
typedef struct {
    uint8_t type;
    int player_id;
    uint8_t success;
} PacketConnectResp;

// Состояние игрока
typedef struct {
    int id;
    char username[32];
    float x, y;
    float vx, vy;
    int team;
    bool alive;
    bool has_flag;
    uint8_t skin_color;
} PlayerState;

// Состояние мира (упрощенное для сети)
typedef struct {
    uint8_t type;
    uint8_t health; // Для разрушаемых блоков
} NetworkBlock;

// Пакет состояния игры (от сервера к клиенту)
typedef struct {
    uint8_t type;
    int tick;
    int score_blue;
    int score_red;
    int flag_progress; // 0-100
    int active_players;
    PlayerState players[MAX_PLAYERS];
    // Блоки отправляем только при изменении, но для простоты первой версии
    // можно отправлять дельту или полные данные редко. 
    // В этом пакете только динамические объекты.
} PacketGameState;

// Пакет ввода (от клиента к серверу)
typedef struct {
    uint8_t type;
    int player_id;
    uint8_t keys; // Битовая маска: 1=Left, 2=Right, 4=Up, 8=Down, 16=Action
    float mouse_x, mouse_y;
    uint8_t action_type; // 0=None, 1=Mine, 2=Place, 3=Jump
    uint8_t block_to_place;
} PacketInput;

// Пакет обновления блока
typedef struct {
    uint8_t type;
    int x, y;
    uint8_t block_type;
} PacketBlockUpdate;

// Пакет обновления флага
typedef struct {
    uint8_t type;
    int carrier_id; // -1 если ни у кого
    float flag_x, flag_y;
    int progress; // 0-100
    int capturing_team; // Кто захватывает (-1 если никто)
} PacketFlagUpdate;

#endif // NETWORK_H
