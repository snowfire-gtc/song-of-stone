// local_server.c - Локальный сервер для одиночной игры
#include "local_server.h"
#include <stdio.h>
#include <string.h>

// Инициализация локального сервера
int local_server_init(LocalServer* local_server, WorldState* world, int port) {
    if (!local_server || !world) {
        return 0;
    }
    
    memset(local_server, 0, sizeof(LocalServer));
    
    // Настройка конфигурации сервера
    local_server->config.port = port;
    local_server->config.max_clients = MAX_CLIENTS;
    local_server->config.tick_rate = SERVER_TICK_RATE;
    local_server->config.friendly_fire = 0;
    local_server->config.pvp_enabled = 0;
    local_server->config.fall_damage = 1;
    local_server->config.difficulty = 1;
    strcpy(local_server->config.server_name, "Local Singleplayer");
    strcpy(local_server->config.motd, "Одиночная игра с локальным сервером");
    
    // Инициализация сервера
    if (!net_server_init(&local_server->server, &local_server->config, world)) {
        fprintf(stderr, "Не удалось инициализировать локальный сервер\n");
        return 0;
    }
    
    local_server->port = port;
    local_server->is_running = 1;
    
    printf("Локальный сервер запущен на порту %d\n", port);
    return 1;
}

// Завершение работы локального сервера
void local_server_shutdown(LocalServer* local_server) {
    if (!local_server || !local_server->is_running) {
        return;
    }
    
    net_server_shutdown(&local_server->server);
    local_server->is_running = 0;
    
    printf("Локальный сервер остановлен\n");
}

// Обновление локального сервера (вызывается каждый кадр)
void local_server_update(LocalServer* local_server, double dt) {
    if (!local_server || !local_server->is_running) {
        return;
    }
    
    net_server_update(&local_server->server, dt);
}

// Проверка, запущен ли локальный сервер
int local_server_is_running(LocalServer* local_server) {
    if (!local_server) {
        return 0;
    }
    return local_server->is_running;
}
