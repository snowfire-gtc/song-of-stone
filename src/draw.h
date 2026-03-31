// src/draw.h
#ifndef DRAW_H
#define DRAW_H

#include "common_game.h"
#include "raylib.h"

// Глобальный счётчик кадров
int get_frame_counter(void);
void increment_frame_counter(void);

// Инициализация шрифтов для UI
void init_ui_font(void);

// Отрисовка фона (параллакс)
void draw_background(WorldState* world);

// Отрисовка всех блоков
void draw_blocks(WorldState* world);

// Отрисовка выпавших предметов
void draw_dropped_items(WorldState* world);

// Отрисовка векторов отладки
void draw_debug_vectors(const WorldState* world);

// Отрисовка флага
void draw_flag(Vector2 pos, Team team, bool carried);

// Отрисовка UI (интерфейс)
void draw_ui(const WorldState* world);

#endif // DRAW_H
