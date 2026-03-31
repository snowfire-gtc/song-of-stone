// src/main.c
#include "raylib.h"
#include "common_game.h"
#include "logic.h"
#include "logic_warrior.h"
#include "logic_archer.h"
#include "logic_worker.h"
#include "logic_bomb.h"
#include "logic_arrow.h"
#include "draw.h"
#include "draw_warrior.h"
#include "draw_archer.h"
#include "draw_worker.h"
#include "draw_bomb.h"
#include "draw_arrow.h"
#include "sound.h"
#include "gen.h"
#include "debug_console.h"
#include "particles.h"
#include "menu.h"
#include "settings.h"
#include "local_server.h"
#include "client.h"
#include <stdio.h>
#include <string.h>

// Глобальная система частиц (объявление, определение в particles.c)
extern ParticleSystem g_particles;

// Глобальные переменные для меню и настроек
Menu g_menu;
Settings g_settings;
GameState g_game_state = GAME_STATE_MENU;  // Глобальное состояние игры для доступа из меню

// Глобальная камера для слежения за игроком
Camera2D g_camera = {0};

// Локальный сервер и клиент для одиночной игры (теперь extern, объявлены в main.h)
LocalServer g_local_server;
GameClient g_client;
int g_is_singleplayer_with_server = false;

int main(void) {
    const int WIN_W = 1280;
    const int WIN_H = 720;
    
    // Инициализация настроек
    settings_init(&g_settings);
    settings_load(&g_settings, "settings.cfg");
    settings_apply_video(&g_settings.video);
    
    InitWindow(g_settings.video.screen_width, g_settings.video.screen_height, "Song of Stone");
    SetTargetFPS(g_settings.video.fps_limit);
    
    // Применение настроек звука
    settings_apply_audio(&g_settings.audio);

    init_sound();
    init_ui_font();
    draw_warrior_init();
    draw_archer_init();
    draw_worker_init();
    debug_console_init();
    particles_init(&g_particles);
    menu_init(&g_menu);

    WorldState* world = gen_world_default();
    
    // Начальное состояние - меню
    GameState game_state = GAME_STATE_MENU;
    bool is_networked = false;
    int local_player_id = 0;
    world->local_player_id = local_player_id;
    
    // Инициализация локального сервера и клиента (для унификации)
    memset(&g_local_server, 0, sizeof(LocalServer));
    memset(&g_client, 0, sizeof(GameClient));
    g_is_singleplayer_with_server = false;

    // Инициализация камеры
    g_camera.target = (Vector2){world->throne_blue.x, world->throne_blue.y};
    g_camera.offset = (Vector2){GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
    g_camera.rotation = 0.0f;
    g_camera.zoom = 1.0f;

    int frame_counter = 0;
    bool debug_console_open = false;
    Character* ch = &world->characters[0];
    
    double dt = 0.0;
    double last_time = GetTime();

    while (!WindowShouldClose()) {
        double current_time = GetTime();
        dt = current_time - last_time;
        last_time = current_time;
        
        // Ограничение dt для стабильности физики
        if (dt > 0.1) dt = 0.1;
        
        // Обновляем глобальное состояние
        g_game_state = game_state;
        
        if (IsKeyPressed(KEY_GRAVE)) {
            debug_console_open = !debug_console_open;
        }
        
        // Обработка состояний игры
        switch (game_state) {
            case GAME_STATE_MENU:
                menu_update(&g_menu, world, dt);
                menu_handle_input(&g_menu, world);
                
                // Если меню скрыто - переходим к игре
                if (!g_menu.visible) {
                    game_state = GAME_STATE_PLAYING;
                }
                break;
                
            case GAME_STATE_PLAYING:
                // Обновление локального сервера (если запущен)
                if (g_is_singleplayer_with_server) {
                    local_server_update(&g_local_server, dt);
                    client_update(&g_client, dt);
                }
                
                // Пауза по ESC
                if (IsKeyPressed(KEY_ESCAPE)) {
                    game_state = GAME_STATE_PAUSED;
                    g_menu.visible = true;
                    g_menu.state = MENU_STATE_PAUSE;
                }
                
                // Обновление камеры - слежение за игроком
                if (world->char_count > 0) {
                    Character* player = &world->characters[world->local_player_id];
                    g_camera.target.x = player->x + 8;  // центр персонажа
                    g_camera.target.y = player->y + 16;
                }
                
                // Обновление логики игры
                Character* player = &world->characters[world->local_player_id];
                
                // Обработка ввода игрока (одиночная игра с локальным сервером)
                if (g_is_singleplayer_with_server && player->hp > 0) {
                    float speed = 200.0f;
                    float jump_force = -350.0f;
                    
                    // Проверка: на земле ли персонаж
                    int block_x = player->x / 16;
                    int block_y = player->y / 16;
                    Block* block_below = logic_get_block(world, block_x, block_y - 1);
                    bool on_ground = (block_below && block_below->type != BLOCK_AIR && 
                                      block_below->type != BLOCK_WATER);
                    
                    // Движение влево/вправо
                    if (IsKeyDown(g_settings.controls.move_left)) {
                        player->vx = -speed;
                        player->facing_right = false;
                    } else if (IsKeyDown(g_settings.controls.move_right)) {
                        player->vx = speed;
                        player->facing_right = true;
                    } else {
                        player->vx = 0;
                    }
                    
                    // Прыжок
                    if (IsKeyPressed(g_settings.controls.jump) && on_ground) {
                        player->vy = jump_force;
                    }
                    
                    // Специфичные действия для классов
                    if (player->type == CHAR_WORKER) {
                        logic_worker_update(player, world, frame_counter);
                        if (IsKeyPressed(KEY_E)) {
                            int bx = player->x / 16, by = player->y / 16;
                            logic_worker_dig_block(player, world, bx, by);
                        }
                    } else if (player->type == CHAR_ARCHER) {
                        logic_archer_update(player, world, frame_counter);
                        // Лазание
                        if (player->is_climbing) {
                            if (IsKeyDown(g_settings.controls.move_up)) {
                                player->y -= 80.0f * GetFrameTime();
                            }
                            if (IsKeyDown(g_settings.controls.move_down)) {
                                player->y += 80.0f * GetFrameTime();
                            }
                        }
                    } else if (player->type == CHAR_WARRIOR) {
                        // Воин может использовать щит
                        if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) {
                            player->is_shield_active = true;
                        } else {
                            player->is_shield_active = false;
                        }
                    }
                }
                
                logic_update(world, frame_counter);
                logic_bomb_update_all(world, frame_counter);
                logic_arrow_update_all(world, frame_counter);
                debug_console_update(world);
                particles_update(&g_particles, dt);
                break;
                
            case GAME_STATE_PAUSED:
                menu_update(&g_menu, world, dt);
                menu_handle_input(&g_menu, world);
                
                // Если меню паузы скрыто - возвращаемся к игре
                if (!g_menu.visible && g_menu.state == MENU_STATE_PAUSE) {
                    game_state = GAME_STATE_PLAYING;
                }
                break;
                
            case GAME_STATE_CONNECTING:
                // Подключение к локальному серверу в одиночной игре
                if (g_is_singleplayer_with_server) {
                    client_update(&g_client, dt);
                    
                    // Проверка успешного подключения
                    if (client_is_connected(&g_client)) {
                        game_state = GAME_STATE_PLAYING;
                        is_networked = true;
                        printf("Успешное подключение к локальному серверу\n");
                    }
                    
                    // Таймаут подключения (5 секунд)
                    static double connect_timer = 0.0;
                    connect_timer += dt;
                    if (connect_timer > 5.0) {
                        fprintf(stderr, "Таймаут подключения к локальному серверу\n");
                        game_state = GAME_STATE_MENU;
                        g_menu.visible = true;
                        g_is_singleplayer_with_server = false;
                        local_server_shutdown(&g_local_server);
                        connect_timer = 0.0;
                    }
                } else {
                    // Старая логика для multiplayer
                    if (frame_counter > 60) {
                        game_state = GAME_STATE_PLAYING;
                        is_networked = true;
                    }
                }
                break;
                
            case GAME_STATE_EXIT:
                CloseWindow();
                return 0;
                
            default:
                break;
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);
        
        // Отрисовка игрового мира (кроме меню)
        if (game_state != GAME_STATE_MENU) {
            // Используем камеру для отрисовки мира
            BeginMode2D(g_camera);
            
            draw_background(world);
            draw_blocks(world);
            draw_dropped_items(world);
            draw_warrior_all(world, frame_counter, world->local_player_id);
            draw_archer_all(world, frame_counter, world->local_player_id);
            draw_worker_all(world, frame_counter, world->local_player_id);
            draw_bomb_all(world, frame_counter);
            draw_arrow_all(world, frame_counter);
            particles_draw(&g_particles);
            
            draw_debug_vectors(world);
            
            EndMode2D();
            
            // UI рисуется без камеры (в экранных координатах)
            draw_ui(world);
            
            if (debug_console_open) {
                draw_debug_console();
            }
        }
        
        // Отрисовка меню поверх всего
        menu_render(&g_menu, world);

        EndDrawing();

        frame_counter++;
    }
    
    // Сохранение настроек перед выходом
    settings_save(&g_settings, "settings.cfg");
    
    // Завершение работы локального сервера (если запущен)
    if (g_is_singleplayer_with_server) {
        local_server_shutdown(&g_local_server);
        client_shutdown(&g_client);
    }

    draw_warrior_unload();
    draw_archer_unload();
    draw_worker_unload();
    draw_bomb_unload();
    draw_arrow_unload();
    sound_unload();
    CloseWindow();
    return 0;
}
