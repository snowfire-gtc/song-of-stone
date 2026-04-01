// draw_ui.h
#ifndef DRAW_UI_H
#define DRAW_UI_H

#include "common_game.h"

void init_ui_font(void);
void init_ui_textures(void);
void unload_ui_textures(void);
void draw_ui(const WorldState* world);

#endif
