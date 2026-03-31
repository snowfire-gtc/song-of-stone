// Net Server Header - Серверная часть сети
#ifndef NET_SERVER_H
#define NET_SERVER_H

#include "common_game.h"
#include "net_protocol.h"

// Сетевые заголовки только для сервера
#ifdef DEDICATED_SERVER
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#define MAX_CLIENTS 4
#define SERVER_TICK_RATE 60

typedef struct {
    int socket_fd;
#ifdef DEDICATED_SERVER
    struct sockaddr_in address;
    socklen_t addr_len;
#else
    uint8_t address_reserved[32];  // Заглушка для клиента
#endif
    int connected;
    int is_bot;
    char name[32];
    double last_packet_time;
    int packets_per_second;
    double last_pps_reset;
    uint32_t last_sequence;
} ClientInfo;

typedef struct {
    int port;
    int max_clients;
    int tick_rate;
    int friendly_fire;
    int pvp_enabled;
    int fall_damage;
    int difficulty;
    char server_name[128];
    char motd[256];
} ServerConfig;

typedef struct {
    int socket_fd;
    int running;
    ServerConfig config;
    WorldState* world;
    
    ClientInfo clients[MAX_CLIENTS];
    Character characters[MAX_CLIENTS];
    int client_count;
    
    // Статистика
    double game_time;
    unsigned long total_ticks;
    double avg_fps;
    Weather weather;
    
    // Буферы
    uint8_t send_buffer[4096];
    uint8_t recv_buffer[4096];
} GameServer;

// Инициализация и завершение
int net_server_init(GameServer* server, ServerConfig* config, WorldState* world);
void net_server_shutdown(GameServer* server);

// Основной цикл
void net_server_update(GameServer* server, double dt);

// Управление клиентами
int net_server_accept_client(GameServer* server);
void net_server_disconnect_client(GameServer* server, int client_id, const char* reason);
void net_server_send_to_client(GameServer* server, int client_id, const void* data, size_t size);
void net_server_broadcast(GameServer* server, int exclude_id, const void* data, size_t size);

// Отправка игровых данных
void net_server_send_snapshot(GameServer* server, int client_id);
void net_server_send_world_state(GameServer* server, int client_id);
void net_server_send_chat(GameServer* server, int client_id, const char* message);

// Обработка входящих пакетов
void net_server_process_packet(GameServer* server, int client_id, PacketHeader* header, uint8_t* payload, size_t payload_size);

// Утилиты
double get_time_seconds(void);

#endif // NET_SERVER_H
