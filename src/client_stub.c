// client_stub.c - Заглушка для клиентской части (если сеть отключена)
#include "client.h"
#include <stdio.h>
#include <string.h>

// Инициализация клиента (заглушка)
int client_init(GameClient* client, const char* server_ip, int port) {
    if (!client) return -1;
    memset(client, 0, sizeof(GameClient));
    printf("[CLIENT] Network stub - no actual connection\n");
    return 0;
}

// Отключение клиента (заглушка)
void client_shutdown(GameClient* client) {
    (void)client;
}

// Отправка ввода (заглушка)
void client_send_input(GameClient* client, PlayerInput* input) {
    (void)client;
    (void)input;
}

// Отправка действия (заглушка)
void client_send_action(GameClient* client, ActionType action, int target_x, int target_y) {
    (void)client;
    (void)action;
    (void)target_x;
    (void)target_y;
}

// Отправка чата (заглушка)
void client_send_chat(GameClient* client, const char* message) {
    (void)client;
    (void)message;
}

// Получение пакетов (заглушка)
int client_receive_packets(GameClient* client) {
    (void)client;
    return 0;
}

// Применение снапшота (заглушка)
void client_apply_snapshot(GameClient* client, PacketSnapshot* snapshot) {
    (void)client;
    (void)snapshot;
}

// Обновление клиента (заглушка)
void client_update(GameClient* client, double dt) {
    (void)client;
    (void)dt;
}

// Проверка подключен ли клиент
int client_is_connected(GameClient* client) {
    (void)client;
    return 0;
}

// Получение задержки (заглушка)
float client_get_latency(GameClient* client) {
    (void)client;
    return 0.0f;
}

// Статус подключения (заглушка)
const char* client_get_connection_status(GameClient* client) {
    (void)client;
    return "disconnected";
}

// Интерполяция персонажей (заглушка)
void client_interpolate_characters(GameClient* client, double alpha) {
    (void)client;
    (void)alpha;
}
