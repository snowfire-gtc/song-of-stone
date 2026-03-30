#ifndef MENU_H
#define MENU_H

#include "raylib.h"
#include "common_game.h"
#include "settings.h"

// Состояния меню
typedef enum {
    MENU_STATE_MAIN,
    MENU_STATE_SETTINGS,
    MENU_STATE_SETTINGS_VIDEO,
    MENU_STATE_SETTINGS_AUDIO,
    MENU_STATE_SETTINGS_CONTROLS,
    MENU_STATE_SETTINGS_APPEARANCE,
    MENU_STATE_NETWORK,
    MENU_STATE_PAUSE
} MenuState;

// Структура меню
typedef struct {
    MenuState state;
    MenuState previous_state; // Для возврата из подменю
    int selected_option;
    int scroll_offset;
    bool visible;
    
    // Поля для ввода
    char server_ip[64];
    int server_port;
    char player_name[32];
    
    // Анимации
    float alpha;
    float scale;
} Menu;

// Инициализация меню
void menu_init(Menu* menu);

// Обновление логики меню
void menu_update(Menu* menu, WorldState* world, double dt);

// Отрисовка меню
void menu_render(Menu* menu, WorldState* world);

// Переключение видимости меню
void menu_toggle(Menu* menu);

// Обработка ввода в меню
void menu_handle_input(Menu* menu, WorldState* world);

// Отрисовка кнопки
bool menu_draw_button(const char* text, Rectangle bounds, bool disabled);

// Отрисовка заголовка
void menu_draw_header(const char* title, Vector2 position);

// Отрисовка ползунка
float menu_draw_slider(const char* label, Rectangle bounds, float value, float min, float max);

// Отрисовка поля ввода
void menu_draw_input_box(const char* label, Rectangle bounds, char* buffer, int max_len, bool active);

#endif // MENU_H
