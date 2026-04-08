#include "menu.h"
#include "main.h"  // Для доступа к g_game_state
#include "settings.h"
#include "local_server.h"
#include "client.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

// Цвета UI
#define MENU_BG_COLOR (Color){0, 0, 0, 200}
#define MENU_BUTTON_COLOR (Color){50, 50, 50, 200}
#define MENU_BUTTON_HOVER (Color){70, 70, 70, 220}
#define MENU_BUTTON_SELECTED (Color){100, 100, 150, 220}
#define MENU_TEXT_COLOR RAYWHITE
#define MENU_TITLE_COLOR GOLD

void menu_init(Menu* menu) {
    menu->state = MENU_STATE_MAIN;
    menu->previous_state = MENU_STATE_MAIN;
    menu->selected_option = 0;
    menu->scroll_offset = 0;
    menu->visible = true; // Показываем меню при старте
    menu->alpha = 1.0f;
    menu->scale = 1.0f;
    
    // Значения по умолчанию
    strcpy(menu->server_ip, "127.0.0.1");
    menu->server_port = 5555;
    strcpy(menu->player_name, "Player");
}

void menu_toggle(Menu* menu) {
    menu->visible = !menu->visible;
    if (menu->visible) {
        menu->alpha = 0.0f;
        menu->scale = 0.9f;
    }
}

bool menu_draw_button(const char* text, Rectangle bounds, bool disabled) {
    Color color = disabled ? GRAY : MENU_BUTTON_COLOR;
    
    Vector2 mouse = GetMousePosition();
    if (CheckCollisionPointRec(mouse, bounds) && !disabled) {
        color = IsMouseButtonPressed(MOUSE_LEFT_BUTTON) ? MENU_BUTTON_SELECTED : MENU_BUTTON_HOVER;
    }
    
    DrawRectangleRec(bounds, color);
    DrawRectangleLinesEx(bounds, 2, disabled ? DARKGRAY : WHITE);
    
    int textWidth = MeasureText(text, 20);
    DrawText(text, 
             bounds.x + (bounds.width - textWidth) / 2,
             bounds.y + (bounds.height - 20) / 2,
             20, disabled ? LIGHTGRAY : MENU_TEXT_COLOR);
    
    return CheckCollisionPointRec(mouse, bounds) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !disabled;
}

void menu_draw_header(const char* title, Vector2 position) {
    DrawText(title, position.x, position.y, 40, MENU_TITLE_COLOR);
    DrawLine(position.x, position.y + 45, position.x + 400, position.y + 45, GOLD);
}

float menu_draw_slider(const char* label, Rectangle bounds, float value, float min, float max) {
    DrawText(label, bounds.x, bounds.y - 25, 16, MENU_TEXT_COLOR);
    
    // Фон слайдера
    DrawRectangleRec(bounds, DARKGRAY);
    DrawRectangleLinesEx(bounds, 2, WHITE);
    
    // Заполнение
    float fill_width = ((value - min) / (max - min)) * bounds.width;
    DrawRectangle(bounds.x, bounds.y, fill_width, bounds.height, BLUE);
    
    // Ползунок
    float handle_x = bounds.x + fill_width - 5;
    DrawRectangle(handle_x, bounds.y - 5, 10, bounds.height + 10, WHITE);
    
    // Значение
    char value_str[32];
    snprintf(value_str, sizeof(value_str), "%.2f", value);
    DrawText(value_str, bounds.x + bounds.width + 10, bounds.y, 16, MENU_TEXT_COLOR);
    
    // Проверка клика
    Vector2 mouse = GetMousePosition();
    if (CheckCollisionPointRec(mouse, bounds) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        float new_value = min + ((mouse.x - bounds.x) / bounds.width) * (max - min);
        return fmaxf(min, fminf(max, new_value));
    }
    
    return value;
}

void menu_draw_input_box(const char* label, Rectangle bounds, char* buffer, int max_len, bool active) {
    DrawText(label, bounds.x, bounds.y - 25, 16, MENU_TEXT_COLOR);
    
    Color bg_color = active ? (Color){60, 60, 80, 255} : (Color){40, 40, 60, 255};
    DrawRectangleRec(bounds, bg_color);
    DrawRectangleLinesEx(bounds, 2, active ? YELLOW : WHITE);
    
    DrawText(buffer, bounds.x + 5, bounds.y + 5, 20, MENU_TEXT_COLOR);
    
    // Курсор - мигание каждые 0.5 секунды
    if (active && (((int)(GetTime() * 2)) % 2 == 0)) {
        int text_width = MeasureText(buffer, 20);
        DrawLine(bounds.x + 5 + text_width, bounds.y + 5,
                 bounds.x + 5 + text_width, bounds.y + 25, WHITE);
    }
}

void menu_update(Menu* menu, WorldState* world, double dt) {
    if (!menu->visible) return;
    
    // Анимация появления
    if (menu->alpha < 1.0f) {
        menu->alpha += dt * 5.0f;
        if (menu->alpha > 1.0f) menu->alpha = 1.0f;
    }
    if (menu->scale < 1.0f) {
        menu->scale += dt * 3.0f;
        if (menu->scale > 1.0f) menu->scale = 1.0f;
    }
    
    // Обработка клавиатуры - Escape работает как кнопка Back
    if (IsKeyPressed(KEY_ESCAPE)) {
        switch (menu->state) {
            case MENU_STATE_MAIN:
                // В главном меню Esc ничего не делает (приложение не закрывается)
                break;
            case MENU_STATE_NETWORK:
            case MENU_STATE_PAUSE:
                // Из этих состояний возвращаемся в предыдущее
                menu->state = menu->previous_state;
                break;
            case MENU_STATE_SETTINGS:
                // Из Settings возвращаемся в главное меню
                menu->state = MENU_STATE_MAIN;
                break;
            case MENU_STATE_SETTINGS_VIDEO:
            case MENU_STATE_SETTINGS_AUDIO:
            case MENU_STATE_SETTINGS_CONTROLS:
            case MENU_STATE_SETTINGS_APPEARANCE:
                // Из подменю настроек возвращаемся в Settings
                menu->state = MENU_STATE_SETTINGS;
                break;
            default:
                // Для любых других состояний используем previous_state
                menu->state = menu->previous_state;
                break;
        }
    }
}

void menu_render_main(Menu* menu, WorldState* world) {
    int screen_w = GetScreenWidth();
    int screen_h = GetScreenHeight();
    
    float btn_width = 300;
    float btn_height = 50;
    float start_x = (screen_w - btn_width) / 2;
    float start_y = screen_h / 2 - 100;
    
    menu_draw_header("SONG OF STONE", (Vector2){start_x + 50, start_y - 80});
    
    if (menu_draw_button("Play Singleplayer", (Rectangle){start_x, start_y, btn_width, btn_height}, false)) {
        // Запуск одиночной игры с локальным сервером
        // Инициализация локального сервера на порту 5555
        
        const int LOCAL_PORT = 5555;
        
        // Создание персонажа игрока
        world->char_count = 1;
        Character* player = &world->characters[0];
        memset(player, 0, sizeof(Character));
        
        // Позиция спавна у синего трона
        player->x = (int)world->throne_blue.x;
        player->y = (int)world->throne_blue.y + 32;
        player->type = CHAR_WORKER;
        player->team = TEAM_BLUE;
        player->hp = PLAYER_MAX_HP;
        player->oxygen = PLAYER_OXYGEN_MAX;
        player->player_id = 0;
        player->anim_state = ANIM_IDLE;
        strcpy(player->name, "Player");
        
        world->local_player_id = 0;
        world->is_multiplayer = 0; // Одиночная игра
        
        // Обновление камеры для слежения за игроком сразу после спавна
        g_camera.target.x = player->x + 8;  // центр персонажа
        g_camera.target.y = player->y + 16;
        
        // Сброс свободного режима камеры при старте игры
        g_camera_free_mode = false;
        g_camera_free_position = g_camera.target;
        
        // Инициализация локального сервера
        if (local_server_init(&g_local_server, world, LOCAL_PORT)) {
            printf("Локальный сервер успешно инициализирован\n");
            
            // Подключение клиента к локальному серверу
            if (client_init(&g_client, "127.0.0.1", LOCAL_PORT)) {
                printf("Клиент подключается к локальному серверу...\n");
                g_is_singleplayer_with_server = true;
                g_game_state = GAME_STATE_CONNECTING;
                menu->visible = false;
            } else {
                fprintf(stderr, "Не удалось подключить клиент к локальному серверу\n");
                local_server_shutdown(&g_local_server);
                g_is_singleplayer_with_server = false;
            }
        } else {
            fprintf(stderr, "Не удалось инициализировать локальный сервер\n");
            // Запуск в старом режиме (без сети)
            g_is_singleplayer_with_server = false;
            g_game_state = GAME_STATE_PLAYING;
            menu->visible = false;
        }
    }
    
    if (menu_draw_button("Multiplayer", (Rectangle){start_x, start_y + 60, btn_width, btn_height}, false)) {
        menu->previous_state = MENU_STATE_MAIN;
        menu->state = MENU_STATE_NETWORK;
    }
    
    if (menu_draw_button("Settings", (Rectangle){start_x, start_y + 120, btn_width, btn_height}, false)) {
        menu->previous_state = MENU_STATE_MAIN;
        menu->state = MENU_STATE_SETTINGS;
    }
    
    if (menu_draw_button("Exit", (Rectangle){start_x, start_y + 180, btn_width, btn_height}, false)) {
        g_game_state = GAME_STATE_EXIT;
    }
}

void menu_render_network(Menu* menu, WorldState* world) {
    int screen_w = GetScreenWidth();
    int screen_h = GetScreenHeight();
    
    float form_width = 400;
    float start_x = (screen_w - form_width) / 2;
    float start_y = screen_h / 2 - 150;
    
    menu_draw_header("Multiplayer", (Vector2){start_x + 50, start_y - 80});
    
    // Поле IP сервера
    menu_draw_input_box("Server IP:", 
                        (Rectangle){start_x, start_y, form_width, 40},
                        menu->server_ip, sizeof(menu->server_ip), false);
    
    // Поле порта
    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", menu->server_port);
    menu_draw_input_box("Port:", 
                        (Rectangle){start_x, start_y + 60, form_width, 40},
                        port_str, sizeof(port_str), false);
    menu->server_port = atoi(port_str);
    
    // Поле имени игрока
    menu_draw_input_box("Player Name:", 
                        (Rectangle){start_x, start_y + 120, form_width, 40},
                        menu->player_name, sizeof(menu->player_name), false);
    
    // Кнопки
    if (menu_draw_button("Connect", (Rectangle){start_x, start_y + 180, form_width/2 - 10, 50}, false)) {
        // Попытка подключения
        printf("Connecting to %s:%d as %s...\n", menu->server_ip, menu->server_port, menu->player_name);
        // Здесь будет вызов net_client_connect()
        g_game_state = GAME_STATE_CONNECTING;
        menu->visible = false;
    }
    
    if (menu_draw_button("Back", (Rectangle){start_x + form_width/2 + 10, start_y + 180, form_width/2 - 10, 50}, false)) {
        menu->state = menu->previous_state;
    }
    
    // Информация
    DrawText("Enter server IP and port to connect", start_x, start_y + 250, 16, LIGHTGRAY);
}

void menu_render_settings(Menu* menu, WorldState* world) {
    int screen_w = GetScreenWidth();
    int screen_h = GetScreenHeight();
    
    float btn_width = 300;
    float btn_height = 50;
    float start_x = (screen_w - btn_width) / 2;
    float start_y = screen_h / 2 - 150;
    
    menu_draw_header("Settings", (Vector2){start_x + 50, start_y - 80});
    
    if (menu_draw_button("Video", (Rectangle){start_x, start_y, btn_width, btn_height}, false)) {
        menu->previous_state = MENU_STATE_SETTINGS;
        menu->state = MENU_STATE_SETTINGS_VIDEO;
    }
    
    if (menu_draw_button("Audio", (Rectangle){start_x, start_y + 60, btn_width, btn_height}, false)) {
        menu->previous_state = MENU_STATE_SETTINGS;
        menu->state = MENU_STATE_SETTINGS_AUDIO;
    }
    
    if (menu_draw_button("Controls", (Rectangle){start_x, start_y + 120, btn_width, btn_height}, false)) {
        menu->previous_state = MENU_STATE_SETTINGS;
        menu->state = MENU_STATE_SETTINGS_CONTROLS;
    }
    
    if (menu_draw_button("Appearance", (Rectangle){start_x, start_y + 180, btn_width, btn_height}, false)) {
        menu->previous_state = MENU_STATE_SETTINGS;
        menu->state = MENU_STATE_SETTINGS_APPEARANCE;
    }
    
    if (menu_draw_button("Back", (Rectangle){start_x, start_y + 240, btn_width, btn_height}, false)) {
        MenuState prev = menu->previous_state;
        // Если previous_state указывает на само Settings, идем в главное меню
        if (prev == MENU_STATE_SETTINGS) {
            prev = MENU_STATE_MAIN;
        }
        menu->state = prev;
    }
}

void menu_render_settings_video(Menu* menu, WorldState* world) {
    int screen_w = GetScreenWidth();
    int screen_h = GetScreenHeight();
    
    float slider_width = 300;
    float start_x = (screen_w - slider_width) / 2;
    float start_y = screen_h / 2 - 150;
    
    menu_draw_header("Video Settings", (Vector2){start_x + 50, start_y - 80});
    
    // Пример настроек (нужно интегрировать с Settings struct)
    static float render_scale = 1.0f;
    render_scale = menu_draw_slider("Render Scale", 
                                    (Rectangle){start_x, start_y, slider_width, 30},
                                    render_scale, 0.5f, 2.0f);
    
    // static int fps_limit = 60; // unused
    // Упрощённый слайдер для FPS
    DrawText("FPS Limit: 60", start_x, start_y + 50, 20, MENU_TEXT_COLOR);
    
    if (menu_draw_button("Back", (Rectangle){start_x, start_y + 200, 150, 50}, false)) {
        menu->state = menu->previous_state;
    }
}

void menu_render_settings_audio(Menu* menu, WorldState* world) {
    int screen_w = GetScreenWidth();
    int screen_h = GetScreenHeight();
    
    float slider_width = 300;
    float start_x = (screen_w - slider_width) / 2;
    float start_y = screen_h / 2 - 150;
    
    menu_draw_header("Audio Settings", (Vector2){start_x + 50, start_y - 80});
    
    static float master_vol = 1.0f;
    static float sfx_vol = 1.0f;
    static float music_vol = 0.8f;
    
    master_vol = menu_draw_slider("Master Volume", 
                                  (Rectangle){start_x, start_y, slider_width, 30},
                                  master_vol, 0.0f, 1.0f);
    
    sfx_vol = menu_draw_slider("Sound Effects", 
                               (Rectangle){start_x, start_y + 60, slider_width, 30},
                               sfx_vol, 0.0f, 1.0f);
    
    music_vol = menu_draw_slider("Music", 
                                 (Rectangle){start_x, start_y + 120, slider_width, 30},
                                 music_vol, 0.0f, 1.0f);
    
    // Применение громкости
    // SetMasterVolume(master_vol); и т.д.
    
    if (menu_draw_button("Back", (Rectangle){start_x, start_y + 200, 150, 50}, false)) {
        menu->state = menu->previous_state;
    }
}

void menu_render_settings_controls(Menu* menu, WorldState* world) {
    int screen_w = GetScreenWidth();
    int screen_h = GetScreenHeight();
    
    menu_draw_header("Controls", (Vector2){100, 100});
    
    DrawText("WASD - Movement", 100, 150, 20, MENU_TEXT_COLOR);
    DrawText("Space - Jump", 100, 180, 20, MENU_TEXT_COLOR);
    DrawText("LMB - Attack/Mine", 100, 210, 20, MENU_TEXT_COLOR);
    DrawText("RMB - Build", 100, 240, 20, MENU_TEXT_COLOR);
    DrawText("E - Use Item", 100, 270, 20, MENU_TEXT_COLOR);
    DrawText("Tab - Inventory", 100, 300, 20, MENU_TEXT_COLOR);
    DrawText("Esc - Menu/Pause", 100, 330, 20, MENU_TEXT_COLOR);
    
    if (menu_draw_button("Back", (Rectangle){100, 400, 150, 50}, false)) {
        menu->state = menu->previous_state;
    }
}

void menu_render_settings_appearance(Menu* menu, WorldState* world) {
    int screen_w = GetScreenWidth();
    int screen_h = GetScreenHeight();
    
    menu_draw_header("Character Appearance", (Vector2){100, 100});
    
    // Превью персонажа
    DrawRectangle(400, 150, 100, 150, BROWN); // Тело
    DrawCircle(450, 120, 30, PINK); // Голова
    
    DrawText("Skin Color: Pink", 100, 350, 20, MENU_TEXT_COLOR);
    DrawText("Hair Color: Brown", 100, 380, 20, MENU_TEXT_COLOR);
    DrawText("Clothing: Blue Shirt", 100, 410, 20, MENU_TEXT_COLOR);
    
    if (menu_draw_button("Random", (Rectangle){100, 450, 150, 50}, false)) {
        // Генерация случайной внешности
    }
    
    if (menu_draw_button("Back", (Rectangle){100, 520, 150, 50}, false)) {
        menu->state = menu->previous_state;
    }
}

void menu_render_pause(Menu* menu, WorldState* world) {
    int screen_w = GetScreenWidth();
    int screen_h = GetScreenHeight();
    
    float btn_width = 300;
    float btn_height = 50;
    float start_x = (screen_w - btn_width) / 2;
    float start_y = screen_h / 2 - 100;
    
    menu_draw_header("PAUSE", (Vector2){start_x + 80, start_y - 80});
    
    // Close & Continue - закрыть меню и продолжить игру
    if (menu_draw_button("Close & Continue", (Rectangle){start_x, start_y, btn_width, btn_height}, false)) {
        menu_toggle(menu);
        g_game_state = GAME_STATE_PLAYING;
    }
    
    // Settings - перейти в настройки
    if (menu_draw_button("Settings", (Rectangle){start_x, start_y + 60, btn_width, btn_height}, false)) {
        menu->previous_state = MENU_STATE_PAUSE;
        menu->state = MENU_STATE_SETTINGS;
    }
    
    // Return to the Main Menu - отключиться от сервера и выйти в главное меню
    if (menu_draw_button("Return to the Main Menu", (Rectangle){start_x, start_y + 120, btn_width, btn_height}, false)) {
        // Отключение от сервера
        if (g_is_singleplayer_with_server) {
            local_server_shutdown(&g_local_server);
            client_shutdown(&g_client);
            g_is_singleplayer_with_server = false;
        }
        menu_toggle(menu);
        g_game_state = GAME_STATE_MENU;
        menu->state = MENU_STATE_MAIN;
    }
    
    // Quit to the Desktop - выйти из игры
    if (menu_draw_button("Quit to the Desktop", (Rectangle){start_x, start_y + 180, btn_width, btn_height}, false)) {
        g_game_state = GAME_STATE_EXIT;
    }
}

void menu_render(Menu* menu, WorldState* world) {
    if (!menu->visible) return;
    
    // Полупрозрачный фон
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), 
                  (Color){0, 0, 0, (int)(150 * menu->alpha)});
    
    BeginScissorMode(0, 0, GetScreenWidth(), GetScreenHeight());
    
    // Сохраняем текущую трансформацию и применяем анимацию
    // (в реальном коде нужно масштабирование через матрицы)
    
    switch (menu->state) {
        case MENU_STATE_MAIN:
            menu_render_main(menu, world);
            break;
        case MENU_STATE_NETWORK:
            menu_render_network(menu, world);
            break;
        case MENU_STATE_SETTINGS:
            menu_render_settings(menu, world);
            break;
        case MENU_STATE_SETTINGS_VIDEO:
            menu_render_settings_video(menu, world);
            break;
        case MENU_STATE_SETTINGS_AUDIO:
            menu_render_settings_audio(menu, world);
            break;
        case MENU_STATE_SETTINGS_CONTROLS:
            menu_render_settings_controls(menu, world);
            break;
        case MENU_STATE_SETTINGS_APPEARANCE:
            menu_render_settings_appearance(menu, world);
            break;
        case MENU_STATE_PAUSE:
            menu_render_pause(menu, world);
            break;
        default:
            break;
    }
    
    EndScissorMode();
}

void menu_handle_input(Menu* menu, WorldState* world __attribute__((unused))) {
    if (!menu->visible) return;
    
    // Обработка ввода для полей ввода (упрощённо)
    // В полной версии нужно отслеживать активное поле и обрабатывать ввод текста
}
