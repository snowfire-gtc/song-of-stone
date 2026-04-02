// draw_ascii.h
#ifndef DRAW_ASCII_H
#define DRAW_ASCII_H

#include "common_game.h"

// Переключение режима ASCII
void ascii_toggle_mode(void);
bool ascii_is_enabled(void);

// Отрисовка мира в ASCII режиме
void ascii_render(WorldState* world);

// Инициализация ASCII рендерера
void ascii_init(void);

#endif // DRAW_ASCII_H
