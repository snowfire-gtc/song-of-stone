// src/main.h - Заголовочный файл для глобальных переменных main.c
#ifndef MAIN_H
#define MAIN_H

#include "common_game.h"
#include "local_server.h"
#include "client.h"

// Глобальное состояние игры (объявлено в main.c)
extern GameState g_game_state;

// Глобальные переменные для сетевого режима в одиночной игре
extern LocalServer g_local_server;
extern GameClient g_client;
extern int g_is_singleplayer_with_server;

// Глобальная камера
extern Camera2D g_camera;
extern bool g_camera_free_mode;
extern Vector2 g_camera_free_position;

#endif // MAIN_H
