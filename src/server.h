// server.h - Улучшенная серверная часть с логированием и управлением состоянием
#ifndef SERVER_H
#define SERVER_H

#include <stdint.h>
#include <stdbool.h>
#include "common_game.h"
#include "net_protocol.h"

#define MAX_CLIENTS 128
#define MAX_LOG_ENTRIES 1024
#define LOG_BUFFER_SIZE 256
#define MAX_ADMIN_COMMANDS 32
#define TICK_RATE 60  // Фиксированный tick rate
#define BLOCK_SIZE 16
#define WORLD_H 64

// Уровни логирования
typedef enum {
    LOG_DEBUG = 0,
    LOG_INFO = 1,
    LOG_WARN = 2,
    LOG_ERROR = 3
} LogLevel;

// События для очереди
typedef enum {
    EVT_NONE = 0,
    EVT_PLAYER_JOIN,
    EVT_PLAYER_LEAVE,
    EVT_PLAYER_ACTION,
    EVT_WORLD_CHANGE
} EventType;

typedef struct {
    EventType type;
    uint32_t tick;
    uint16_t player_id;
    union {
        char name[MAX_NAME_LEN];
        struct { int x, y, block_type; } block_change;
        struct { float damage; int source_id; } action;
    } data;
} GameEvent;

// Запись лога
typedef struct {
    uint32_t timestamp;
    LogLevel level;
    char message[LOG_BUFFER_SIZE];
} LogEntry;

// Статистика сервера
typedef struct {
    uint32_t total_ticks;
    uint32_t packets_sent;
    uint32_t packets_received;
    uint32_t players_joined;
    uint32_t players_left;
    uint32_t events_processed;
    double avg_tick_time_ms;
} ServerStats;

// Клиент
typedef struct {
    bool active;
    struct sockaddr_in addr;
    socklen_t addr_len;
    uint16_t player_id;
    PlayerInput last_input;
    uint32_t last_input_tick;
    uint32_t connect_tick;
    uint32_t packets_sent;
    uint32_t packets_received;
    char name[MAX_NAME_LEN];
    bool is_admin;
} Client;

// Серверное состояние
typedef struct {
    int sock;
    bool running;
    uint32_t current_tick;
    uint32_t last_snapshot_tick;
    
    WorldState world;
    Client clients[MAX_CLIENTS];
    int client_count;
    int char_count;  // добавлено для совместимости
    
    // Очередь событий
    GameEvent event_queue[256];
    int event_head;
    int event_tail;
    int event_count;
    
    // Логирование
    LogEntry log_buffer[MAX_LOG_ENTRIES];
    int log_head;
    int log_tail;
    int log_count;
    LogLevel min_log_level;
    
    // Статистика
    ServerStats stats;
    
    // Конфигурация
    bool physics_enabled;
    bool pvp_enabled;
    int max_players;
} ServerState;

// Инициализация и запуск
void server_init(ServerState* srv, int port);
void server_run(ServerState* srv);
void server_shutdown(ServerState* srv);

// Обработка пакетов
void server_handle_packet(ServerState* srv, uint8_t* data, int len, 
                          struct sockaddr_in* addr, socklen_t addr_len);

// Игровой цикл
void server_update(ServerState* srv);
void server_send_snapshots(ServerState* srv);

// Логирование
void server_log(ServerState* srv, LogLevel level, const char* fmt, ...);
void server_print_logs(ServerState* srv, LogLevel min_level);
void server_save_logs(ServerState* srv, const char* filename);

// Админ команды
bool server_handle_admin_command(ServerState* srv, const char* cmd);
void server_list_commands(ServerState* srv);

// События
void server_push_event(ServerState* srv, GameEvent* evt);
void server_process_events(ServerState* srv);

// Статистика
void server_print_stats(ServerState* srv);
void server_reset_stats(ServerState* srv);

#endif // SERVER_H