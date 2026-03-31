// client.h - Клиентская часть сети
#ifndef CLIENT_H
#define CLIENT_H

#include "common_game.h"
#include "net_protocol.h"

#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#define CLIENT_TICK_RATE 60
#define INTERPOLATION_DELAY 0.1f  // задержка интерполяции (сек)

typedef struct {
    int socket_fd;
    struct sockaddr_in server_addr;
    socklen_t addr_len;
    int connected;
    int player_id;
    char server_name[128];
    
    // Буферы
    uint8_t send_buffer[4096];
    uint8_t recv_buffer[4096];
    
    // Локальное состояние мира (интерполированное)
    WorldState local_world;
    
    // Ввод игрока
    PlayerInput current_input;
    PlayerInput last_input;
    
    // Таймеры
    double last_send_time;
    double last_recv_time;
    double reconnect_timer;
    
    // Статистика
    int packets_sent;
    int packets_received;
    float avg_latency;
    int sequence_errors;
    
    // Интерполяция
    float interpolation_delay;
    uint32_t last_snapshot_tick;
    uint32_t pending_snapshot_tick;
} GameClient;

// Инициализация и завершение
int client_init(GameClient* client, const char* server_ip, int port);
void client_shutdown(GameClient* client);

// Основной цикл
void client_update(GameClient* client, double dt);

// Отправка данных
void client_send_input(GameClient* client, PlayerInput* input);
void client_send_action(GameClient* client, ActionType action, int target_x, int target_y);
void client_send_chat(GameClient* client, const char* message);

// Получение данных
int client_receive_packets(GameClient* client);

// Обработка снапшотов
void client_apply_snapshot(GameClient* client, PacketSnapshot* snapshot);
void client_interpolate_characters(GameClient* client, double alpha);

// Утилиты
int client_is_connected(GameClient* client);
float client_get_latency(GameClient* client);
const char* client_get_connection_status(GameClient* client);

#endif // CLIENT_H
