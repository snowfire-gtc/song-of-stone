// local_server.h - Локальный сервер для одиночной игры
#ifndef LOCAL_SERVER_H
#define LOCAL_SERVER_H

#include "net_server.h"
#include "client.h"

// Структура для управления локальным сервером в одиночной игре
typedef struct {
    GameServer server;
    ServerConfig config;
    int is_running;
    int port;
} LocalServer;

// Инициализация локального сервера
int local_server_init(LocalServer* local_server, WorldState* world, int port);

// Завершение работы локального сервера
void local_server_shutdown(LocalServer* local_server);

// Обновление локального сервера (вызывается каждый кадр)
void local_server_update(LocalServer* local_server, double dt);

// Проверка, запущен ли локальный сервер
int local_server_is_running(LocalServer* local_server);

#endif // LOCAL_SERVER_H
