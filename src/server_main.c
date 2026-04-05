// Dedicated Server for Song of Stone
// Работает без графики, только консоль и сеть

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>

#include "common_game.h"
#include "net_server.h"
#include "logic.h"

#define SERVER_PORT 27015
#define TICK_RATE 60
#define SAVE_INTERVAL 300  // Сохранение каждые 5 минут

static volatile int running = 1;
static GameServer server;
static World world;
static ServerConfig config;

void signal_handler(int sig) {
    printf("\nПолучен сигнал %d, завершение работы...\n", sig);
    running = 0;
}

int load_or_create_world(const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (f) {
        printf("Загрузка мира из %s...\n", filename);
        fread(&world, sizeof(World), 1, f);
        fclose(f);
        return 1;
    } else {
        printf("Мир не найден, генерация нового...\n");
        world_generate(&world, WORLD_WIDTH, WORLD_HEIGHT, SEED_RANDOM);
        return 0;
    }
}

int save_world(const char* filename) {
    FILE* f = fopen(filename, "wb");
    if (!f) {
        perror("Не удалось сохранить мир");
        return 0;
    }
    fwrite(&world, sizeof(World), 1, f);
    fclose(f);
    printf("Мир сохранён в %s\n", filename);
    return 1;
}

void print_help() {
    printf("\n=== Команды сервера ===\n");
    printf("  help              - Показать эту справку\n");
    printf("  status            - Статус сервера (игроки, память)\n");
    printf("  list              - Список игроков\n");
    printf("  kick <id>         - Кикнуть игрока по ID\n");
    printf("  ban <id>          - Забанить игрока по ID\n");
    printf("  save              - Сохранить мир вручную\n");
    printf("  reload            - Перезагрузить мир из файла\n");
    printf("  broadcast <msg>   - Отправить сообщение всем игрокам\n");
    printf("  settime <hour>    - Установить время (0-23)\n");
    printf("  weather <type>    - Установить погоду (clear/rain/snow)\n");
    printf("  quit              - Остановить сервер с сохранением\n");
    printf("=====================\n\n");
}

void process_console_command(const char* input) {
    char cmd[256], arg[256];
    int value;
    
    if (sscanf(input, "%255s %255s", cmd, arg) < 1) return;
    
    if (strcmp(cmd, "help") == 0) {
        print_help();
    }
    else if (strcmp(cmd, "status") == 0) {
        printf("\n=== Статус сервера ===\n");
        printf("Игроков: %d/%d\n", server.client_count, MAX_CLIENTS);
        printf("Время в игре: %d:%02d\n", server.game_time / 3600, (server.game_time % 3600) / 60);
        printf("Погода: %s\n", server.weather == WEATHER_RAIN ? "дождь" : 
                              server.weather == WEATHER_SNOW ? "снег" : "ясно");
        printf("Тиков обработано: %lu\n", server.total_ticks);
        printf("Средний FPS: %.1f\n", server.avg_fps);
        printf("======================\n\n");
    }
    else if (strcmp(cmd, "list") == 0) {
        printf("\n=== Игроки ===\n");
        int count = 0;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (server.clients[i].connected && !server.clients[i].is_bot) {
                Character* c = &server.characters[i];
                printf("  [%d] %s (Команда: %s, HP: %d/%d, Счёт: %d)\n",
                       i, c->name,
                       c->team == TEAM_BLUE ? "Синие" : "Красные",
                       c->hp, CHARACTER_MAX_HP, c->score);
                count++;
            }
        }
        if (count == 0) printf("  Нет активных игроков\n");
        printf("================\n\n");
    }
    else if (strcmp(cmd, "kick") == 0 && sscanf(arg, "%d", &value) == 1) {
        if (value >= 0 && value < MAX_CLIENTS) {
            printf("Игрок %d кикнут\n", value);
            net_server_disconnect_client(&server, value, "Kicked by server");
        } else {
            printf("Неверный ID игрока\n");
        }
    }
    else if (strcmp(cmd, "ban") == 0 && sscanf(arg, "%d", &value) == 1) {
        if (value >= 0 && value < MAX_CLIENTS) {
            printf("Игрок %d забанен\n", value);
            // Добавляем в список банов по IP адресу
            if (server.clients[value].connected) {
                char ban_ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &server.clients[value].address.sin_addr, ban_ip, sizeof(ban_ip));
                printf("Забанен IP адрес: %s\n", ban_ip);
                // В полной реализации здесь нужно добавить IP в файл banlist.txt
                // для персистентного хранения банов
            }
            net_server_disconnect_client(&server, value, "Banned by server");
        } else {
            printf("Неверный ID игрока\n");
        }
    }
    else if (strcmp(cmd, "save") == 0) {
        save_world("world.dat");
    }
    else if (strcmp(cmd, "reload") == 0) {
        if (load_or_create_world("world.dat")) {
            printf("Мир перезапущен\n");
            // Уведомить всех игроков о перезагрузке мира
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (server.clients[i].connected) {
                    net_server_send_chat(&server, i, "SERVER: Мир перезапущен!");
                }
            }
        }
    }
    else if (strcmp(cmd, "broadcast") == 0) {
        // Извлечь сообщение после команды broadcast
        const char* msg = input + strlen("broadcast ");
        while (*msg == ' ') msg++;
        if (*msg) {
            printf("[Broadcast] %s\n", msg);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (server.clients[i].connected) {
                    net_server_send_chat(&server, i, msg);
                }
            }
        }
    }
    else if (strcmp(cmd, "settime") == 0 && sscanf(arg, "%d", &value) == 1) {
        if (value >= 0 && value < 24) {
            server.game_time = value * 3600;
            printf("Время установлено на %02d:00\n", value);
        } else {
            printf("Время должно быть от 0 до 23\n");
        }
    }
    else if (strcmp(cmd, "weather") == 0) {
        if (strcmp(arg, "clear") == 0) {
            server.weather = WEATHER_CLEAR;
            printf("Погода: ясно\n");
        } else if (strcmp(arg, "rain") == 0) {
            server.weather = WEATHER_RAIN;
            printf("Погода: дождь\n");
        } else if (strcmp(arg, "snow") == 0) {
            server.weather = WEATHER_SNOW;
            printf("Погода: снег\n");
        } else {
            printf("Неверный тип погоды (clear/rain/snow)\n");
        }
    }
    else if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "exit") == 0) {
        printf("Сохранение и остановка сервера...\n");
        save_world("world.dat");
        running = 0;
    }
    else {
        printf("Неизвестная команда: %s (введите 'help' для справки)\n", cmd);
    }
}

void init_server_config(ServerConfig* cfg) {
    cfg->port = SERVER_PORT;
    cfg->max_clients = MAX_CLIENTS;
    cfg->tick_rate = TICK_RATE;
    cfg->friendly_fire = 0;  // По умолчанию свой не бьёт
    cfg->pvp_enabled = 1;
    cfg->fall_damage = 1;
    cfg->difficulty = DIFFICULTY_NORMAL;
    strncpy(cfg->server_name, "Song of Stone Dedicated Server", sizeof(cfg->server_name));
    strncpy(cfg->motd, "Добро пожаловать на сервер!", sizeof(cfg->motd));
}

int main(int argc, char** argv) {
    printf("===========================================\n");
    printf("  Song of Stone - Dedicated Server\n");
    printf("  Версия: %s\n", GAME_VERSION);
    printf("===========================================\n\n");
    
    // Обработка сигналов
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Инициализация конфигурации
    init_server_config(&config);
    
    // Парсинг аргументов командной строки
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 && i+1 < argc) {
            config.port = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "-max") == 0 && i+1 < argc) {
            config.max_clients = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "-name") == 0 && i+1 < argc) {
            strncpy(config.server_name, argv[++i], sizeof(config.server_name));
        }
        else if (strcmp(argv[i], "-nofall") == 0) {
            config.fall_damage = 0;
        }
        else if (strcmp(argv[i], "-nopvp") == 0) {
            config.pvp_enabled = 0;
        }
        else if (strcmp(argv[i], "-seed") == 0 && i+1 < argc) {
            world.seed = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "-help") == 0) {
            printf("Использование: %s [опции]\n", argv[0]);
            printf("Опции:\n");
            printf("  -p <port>       Порт сервера (по умолчанию %d)\n", SERVER_PORT);
            printf("  -max <num>      Максимум игроков (по умолчанию %d)\n", MAX_CLIENTS);
            printf("  -name <name>    Название сервера\n");
            printf("  -nofall         Отключить урон от падения\n");
            printf("  -nopvp          Отключить PvP между игроками одной команды\n");
            printf("  -seed <num>     Seed для генерации мира\n");
            printf("  -help           Показать эту справку\n");
            return 0;
        }
    }
    
    printf("Настройки сервера:\n");
    printf("  Порт: %d\n", config.port);
    printf("  Макс. игроков: %d\n", config.max_clients);
    printf("  Название: %s\n", config.server_name);
    printf("  PvP: %s\n", config.pvp_enabled ? "включено" : "выключено");
    printf("  Урон от падения: %s\n", config.fall_damage ? "включён" : "выключен");
    printf("\n");
    
    // Инициализация случайных чисел
    srand(time(NULL));
    
    // Загрузка или создание мира
    load_or_create_world("world.dat");
    
    // Инициализация сервера
    if (!net_server_init(&server, &config, &world)) {
        fprintf(stderr, "Ошибка инициализации сервера!\n");
        return 1;
    }
    
    printf("Сервер запущен и слушает порт %d...\n", config.port);
    printf("Введите 'help' для списка команд.\n\n");
    
    // Основной цикл сервера
    double last_tick = get_time_seconds();
    double last_save = last_tick;
    double last_console_check = last_tick;
    char console_input[512] = {0};
    int console_pos = 0;
    
    while (running) {
        double now = get_time_seconds();
        double dt = now - last_tick;
        
        // Обновление сети (приём/отправка пакетов)
        net_server_update(&server, dt);
        
        // Игровая логика с фиксированным шагом
        static double accumulator = 0.0;
        accumulator += dt;
        
        while (accumulator >= 1.0 / TICK_RATE) {
            logic_update_server(&server, &world, 1.0 / TICK_RATE);
            accumulator -= 1.0 / TICK_RATE;
        }
        
        // Автосохранение мира
        if (now - last_save >= SAVE_INTERVAL) {
            save_world("world.dat");
            last_save = now;
        }
        
        // Опрос консоли (неблокирующий)
        if (now - last_console_check >= 0.1) {
            // Простая реализация: читаем по символу если есть
            // Для полноценной консоли можно использовать ncurses
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(STDIN_FILENO, &fds);
            
            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 0;
            
            if (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0) {
                char c = getchar();
                if (c == '\n') {
                    console_input[console_pos] = '\0';
                    if (console_pos > 0) {
                        process_console_command(console_input);
                    }
                    console_pos = 0;
                    printf("> ");
                    fflush(stdout);
                }
                else if (c == 127 || c == '\b') { // Backspace
                    if (console_pos > 0) {
                        console_pos--;
                        printf("\b \b");
                        fflush(stdout);
                    }
                }
                else if (console_pos < sizeof(console_input) - 1 && c >= 32) {
                    console_input[console_pos++] = c;
                    putchar(c);
                    fflush(stdout);
                }
            }
            last_console_check = now;
        }
        
        last_tick = now;
        
        // Небольшая задержка чтобы не грузить CPU
        usleep(1000);
    }
    
    // Очистка
    printf("\nОстановка сервера...\n");
    save_world("world.dat");
    net_server_shutdown(&server);
    printf("Сервер остановлен.\n");
    
    return 0;
}
