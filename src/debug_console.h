// debug_console.h
#ifndef DEBUG_CONSOLE_H
#define DEBUG_CONSOLE_H

#include "common_game.h"

// Инициализация консоли
void debug_console_init(void);

// Обновление (ввод, логика)
void debug_console_update(WorldState* world);

// Отрисовка
void draw_debug_console(void);

// Отрисовка отладочных векторов (на основном экране)
void draw_debug_vectors(const WorldState* world);

// Включить/выключить отладочный режим
extern bool debug_mode_enabled;

#endif // DEBUG_CONSOLE_H
