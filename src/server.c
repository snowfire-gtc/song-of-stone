// server.c - Улучшенная серверная часть с логированием, очередью событий и админ-командами
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <ctype.h>

#include "server.h"
#include "common_game.h"
#include "net_protocol.h"
#include "gen.h"

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

// Вспомогательные функции для логирования
static const char* log_level_str(LogLevel level) {
    switch (level) {
        case LOG_DEBUG: return "DEBUG";
        case LOG_INFO:  return "INFO";
        case LOG_WARN:  return "WARN";
        case LOG_ERROR: return "ERROR";
        default:        return "UNKNOWN";
    }
}

void server_log(ServerState* srv, LogLevel level, const char* fmt, ...) {
    if (level < srv->min_log_level) return;
    
    LogEntry* entry = &srv->log_buffer[srv->log_head];
    entry->timestamp = srv->current_tick;
    entry->level = level;
    
    va_list args;
    va_start(args, fmt);
    vsnprintf(entry->message, LOG_BUFFER_SIZE, fmt, args);
    va_end(args);
    
    srv->log_head = (srv->log_head + 1) % MAX_LOG_ENTRIES;
    if (srv->log_count < MAX_LOG_ENTRIES) {
        srv->log_count++;
    } else {
        srv->log_tail = (srv->log_tail + 1) % MAX_LOG_ENTRIES;
    }
    
    // Вывод в консоль
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    printf("[%02d:%02d:%02d][%s] %s\n", 
           t->tm_hour, t->tm_min, t->tm_sec,
           log_level_str(level), entry->message);
}

void server_print_logs(ServerState* srv, LogLevel min_level) {
    int idx = srv->log_tail;
    int count = srv->log_count;
    
    printf("\n=== SERVER LOGS ===\n");
    for (int i = 0; i < count; i++) {
        LogEntry* entry = &srv->log_buffer[idx];
        if (entry->level >= min_level) {
            printf("[%s][T:%u] %s\n", 
                   log_level_str(entry->level), 
                   entry->timestamp, 
                   entry->message);
        }
        idx = (idx + 1) % MAX_LOG_ENTRIES;
    }
    printf("===================\n\n");
}

void server_save_logs(ServerState* srv, const char* filename) {
    FILE* f = fopen(filename, "w");
    if (!f) {
        server_log(srv, LOG_ERROR, "Failed to save logs to %s", filename);
        return;
    }
    
    int idx = srv->log_tail;
    for (int i = 0; i < srv->log_count; i++) {
        LogEntry* entry = &srv->log_buffer[idx];
        fprintf(f, "[%u][%s] %s\n", 
                entry->timestamp, 
                log_level_str(entry->level), 
                entry->message);
        idx = (idx + 1) % MAX_LOG_ENTRIES;
    }
    
    fclose(f);
    server_log(srv, LOG_INFO, "Logs saved to %s", filename);
}

// Очередь событий
void server_push_event(ServerState* srv, GameEvent* evt) {
    if (srv->event_count >= 256) {
        server_log(srv, LOG_WARN, "Event queue full, dropping event");
        return;
    }
    
    srv->event_queue[srv->event_head] = *evt;
    srv->event_head = (srv->event_head + 1) % 256;
    srv->event_count++;
    srv->stats.events_processed++;
    
    server_log(srv, LOG_DEBUG, "Event pushed: type=%d, player=%d", 
               evt->type, evt->player_id);
}

void server_process_events(ServerState* srv) {
    while (srv->event_count > 0) {
        GameEvent* evt = &srv->event_queue[srv->event_tail];
        
        switch (evt->type) {
            case EVT_PLAYER_JOIN:
                server_log(srv, LOG_INFO, "Player '%s' joined (ID: %d)", 
                          evt->data.name, evt->player_id);
                break;
                
            case EVT_PLAYER_LEAVE:
                server_log(srv, LOG_INFO, "Player ID %d left the game", 
                          evt->player_id);
                break;
                
            case EVT_PLAYER_ACTION:
                server_log(srv, LOG_DEBUG, "Player %d action: damage=%.1f from %d",
                          evt->player_id, evt->data.action.damage, 
                          evt->data.action.source_id);
                break;
                
            case EVT_WORLD_CHANGE:
                server_log(srv, LOG_DEBUG, "World changed at (%d,%d): block=%d",
                          evt->data.block_change.x, evt->data.block_change.y,
                          evt->data.block_change.block_type);
                break;
                
            default:
                break;
        }
        
        srv->event_tail = (srv->event_tail + 1) % 256;
        srv->event_count--;
    }
}

// Статистика
void server_print_stats(ServerState* srv) {
    printf("\n=== SERVER STATISTICS ===\n");
    printf("Total ticks:        %u\n", srv->stats.total_ticks);
    printf("Packets sent:       %u\n", srv->stats.packets_sent);
    printf("Packets received:   %u\n", srv->stats.packets_received);
    printf("Players joined:     %u\n", srv->stats.players_joined);
    printf("Players left:       %u\n", srv->stats.players_left);
    printf("Events processed:   %u\n", srv->stats.events_processed);
    printf("Avg tick time:      %.2f ms\n", srv->stats.avg_tick_time_ms);
    printf("Active clients:     %d / %d\n", srv->client_count, srv->max_players);
    printf("=========================\n\n");
}

void server_reset_stats(ServerState* srv) {
    memset(&srv->stats, 0, sizeof(ServerStats));
    server_log(srv, LOG_INFO, "Statistics reset");
}

// Админ команды
typedef struct {
    const char* cmd;
    const char* desc;
    void (*handler)(ServerState* srv, const char* args);
} AdminCommand;

static void cmd_help(ServerState* srv, const char* args) {
    printf("\nAvailable commands:\n");
    printf("  help              - Show this help\n");
    printf("  stats             - Show server statistics\n");
    printf("  logs [level]      - Show logs (DEBUG/INFO/WARN/ERROR)\n");
    printf("  save_logs         - Save logs to file\n");
    printf("  list_players      - List connected players\n");
    printf("  kick <id>         - Kick player by ID\n");
    printf("  set_max <num>     - Set max players\n");
    printf("  toggle_physics    - Toggle physics\n");
    printf("  toggle_pvp        - Toggle PvP\n");
    printf("  reset_stats       - Reset statistics\n");
    printf("  quit              - Shutdown server\n");
    printf("\n");
}

static void cmd_stats(ServerState* srv, const char* args) {
    server_print_stats(srv);
}

static void cmd_logs(ServerState* srv, const char* args) {
    LogLevel level = LOG_INFO;
    if (strcmp(args, "DEBUG") == 0) level = LOG_DEBUG;
    else if (strcmp(args, "WARN") == 0) level = LOG_WARN;
    else if (strcmp(args, "ERROR") == 0) level = LOG_ERROR;
    
    server_print_logs(srv, level);
}

static void cmd_save_logs(ServerState* srv, const char* args) {
    char filename[64];
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    snprintf(filename, sizeof(filename), "server_log_%04d%02d%02d_%02d%02d.txt",
             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
             t->tm_hour, t->tm_min);
    server_save_logs(srv, filename);
}

static void cmd_list_players(ServerState* srv, const char* args) {
    printf("\nConnected players:\n");
    int count = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (srv->clients[i].active) {
            printf("  ID %3d: %-20s [%s] - Packets: sent=%u, recv=%u\n",
                   srv->clients[i].player_id,
                   srv->clients[i].name,
                   srv->clients[i].is_admin ? "ADMIN" : "PLAYER",
                   srv->clients[i].packets_sent,
                   srv->clients[i].packets_received);
            count++;
        }
    }
    printf("Total: %d players\n\n", count);
}

static void cmd_kick(ServerState* srv, const char* args) {
    int id = atoi(args);
    if (id < 0 || id >= MAX_CLIENTS) {
        printf("Invalid player ID\n");
        return;
    }
    
    if (srv->clients[id].active) {
        srv->clients[id].active = false;
        srv->client_count--;
        srv->stats.players_left++;
        
        GameEvent evt = {0};
        evt.type = EVT_PLAYER_LEAVE;
        evt.player_id = id;
        server_push_event(srv, &evt);
        
        server_log(srv, LOG_INFO, "Player %d kicked by admin", id);
    } else {
        printf("Player %d not found\n", id);
    }
}

static void cmd_set_max(ServerState* srv, const char* args) {
    int max = atoi(args);
    if (max > 0 && max <= MAX_CLIENTS) {
        srv->max_players = max;
        server_log(srv, LOG_INFO, "Max players set to %d", max);
    } else {
        printf("Invalid max players value (1-%d)\n", MAX_CLIENTS);
    }
}

static void cmd_toggle_physics(ServerState* srv, const char* args) {
    srv->physics_enabled = !srv->physics_enabled;
    server_log(srv, LOG_INFO, "Physics %s", srv->physics_enabled ? "enabled" : "disabled");
}

static void cmd_toggle_pvp(ServerState* srv, const char* args) {
    srv->pvp_enabled = !srv->pvp_enabled;
    server_log(srv, LOG_INFO, "PvP %s", srv->pvp_enabled ? "enabled" : "disabled");
}

static void cmd_reset_stats(ServerState* srv, const char* args) {
    server_reset_stats(srv);
}

static void cmd_quit(ServerState* srv, const char* args) {
    server_log(srv, LOG_INFO, "Server shutdown requested by admin");
    srv->running = false;
}

static AdminCommand admin_commands[] = {
    {"help", "Show help", cmd_help},
    {"stats", "Show statistics", cmd_stats},
    {"logs", "Show logs", cmd_logs},
    {"save_logs", "Save logs to file", cmd_save_logs},
    {"list_players", "List connected players", cmd_list_players},
    {"kick", "Kick player by ID", cmd_kick},
    {"set_max", "Set max players", cmd_set_max},
    {"toggle_physics", "Toggle physics", cmd_toggle_physics},
    {"toggle_pvp", "Toggle PvP", cmd_toggle_pvp},
    {"reset_stats", "Reset statistics", cmd_reset_stats},
    {"quit", "Shutdown server", cmd_quit},
    {NULL, NULL, NULL}
};

bool server_handle_admin_command(ServerState* srv, const char* cmd) {
    char cmd_copy[256];
    strncpy(cmd_copy, cmd, sizeof(cmd_copy) - 1);
    cmd_copy[sizeof(cmd_copy) - 1] = '\0';
    
    // Разделить команду и аргументы
    char* args = strchr(cmd_copy, ' ');
    if (args) {
        *args = '\0';
        args++;
        while (*args && isspace(*args)) args++;
    }
    
    // Найти команду
    for (int i = 0; admin_commands[i].cmd != NULL; i++) {
        if (strcmp(cmd_copy, admin_commands[i].cmd) == 0) {
            admin_commands[i].handler(srv, args ? args : "");
            return true;
        }
    }
    
    return false;
}

void server_list_commands(ServerState* srv) {
    cmd_help(srv, "");
}

// Инициализация сервера
void server_init(ServerState* srv, int port) {
    memset(srv, 0, sizeof(ServerState));
    
    srv->sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (srv->sock < 0) {
        perror("Socket creation failed");
        exit(1);
    }
    
    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(srv->sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(1);
    }
    
    srv->world = gen_world_default();
    srv->running = true;
    srv->current_tick = 0;
    srv->client_count = 0;
    srv->max_players = MAX_CLIENTS;
    srv->physics_enabled = true;
    srv->pvp_enabled = true;
    srv->min_log_level = LOG_INFO;
    
    server_log(srv, LOG_INFO, "Server started on port %d", port);
    server_log(srv, LOG_INFO, "Max players: %d", srv->max_players);
    server_log(srv, LOG_INFO, "Physics: %s", srv->physics_enabled ? "ON" : "OFF");
    server_log(srv, LOG_INFO, "PvP: %s", srv->pvp_enabled ? "ON" : "OFF");
}

// Обработка входящего пакета
void server_handle_packet(ServerState* srv, uint8_t* data, int len, 
                          struct sockaddr_in* addr, socklen_t addr_len) {
    if (len < 1) return;
    
    NetworkPacket* pkt = (NetworkPacket*)data;
    srv->stats.packets_received++;
    
    if (pkt->type == PKT_HELLO) {
        // Найти свободного клиента
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (!srv->clients[i].active && srv->client_count < srv->max_players) {
                srv->clients[i].active = true;
                srv->clients[i].addr = *addr;
                srv->clients[i].addr_len = addr_len;
                srv->clients[i].player_id = i;
                srv->clients[i].connect_tick = srv->current_tick;
                strncpy(srv->clients[i].name, pkt->hello_name, MAX_NAME_LEN - 1);
                srv->clients[i].name[MAX_NAME_LEN - 1] = '\0';
                
                // Инициализация персонажа
                Character* ch = &srv->world.characters[i];
                ch->player_id = i;
                ch->team = (i % 2 == 0) ? TEAM_BLUE : TEAM_RED;
                ch->hp = PLAYER_MAX_HP;
                ch->x = (srv->world.throne_blue.x + srv->world.throne_red.x) / 2;
                ch->y = srv->world.throne_blue.y;
                strncpy(ch->name, pkt->hello_name, MAX_NAME_LEN - 1);
                
                srv->client_count++;
                srv->char_count = srv->client_count;
                srv->stats.players_joined++;
                
                // Отправить подтверждение
                sendto(srv->sock, pkt, sizeof(NetworkPacket), 0,
                       (struct sockaddr*)addr, addr_len);
                srv->clients[i].packets_sent++;
                srv->stats.packets_sent++;
                
                // Событие присоединения
                GameEvent evt = {0};
                evt.type = EVT_PLAYER_JOIN;
                evt.player_id = i;
                strncpy(evt.data.name, pkt->hello_name, MAX_NAME_LEN - 1);
                server_push_event(srv, &evt);
                
                server_log(srv, LOG_INFO, "Player '%s' joined as ID %d (Team: %s)", 
                          pkt->hello_name, i, ch->team == TEAM_BLUE ? "BLUE" : "RED");
                break;
            }
        }
    }
    else if (pkt->type == PKT_INPUT) {
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (srv->clients[i].active && srv->clients[i].player_id == pkt->player_id) {
                srv->clients[i].last_input = pkt->input;
                srv->clients[i].last_input_tick = srv->current_tick;
                srv->clients[i].packets_received++;
                break;
            }
        }
    }
    else if (pkt->type == PKT_DISCONNECT) {
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (srv->clients[i].active && srv->clients[i].player_id == pkt->player_id) {
                srv->clients[i].active = false;
                srv->client_count--;
                srv->stats.players_left++;
                
                GameEvent evt = {0};
                evt.type = EVT_PLAYER_LEAVE;
                evt.player_id = i;
                server_push_event(srv, &evt);
                
                server_log(srv, LOG_INFO, "Player '%s' (ID %d) disconnected", 
                          srv->clients[i].name, i);
                break;
            }
        }
    }
}

// Обновление игры на сервере
void server_update(ServerState* srv) {
    struct timeval start, end;
    gettimeofday(&start, NULL);
    
    // Применить ввод от всех клиентов
    for (int i = 0; i < srv->char_count; i++) {
        Character* ch = &srv->world.characters[i];
        if (ch->hp <= 0) continue;
        
        Client* client = &srv->clients[ch->player_id];
        if (!client->active) continue;
        
        PlayerInput* input = &client->last_input;
        
        // Движение
        if (input->move_left) ch->x -= 2;
        if (input->move_right) ch->x += 2;
        if (input->jump && ch->vy == 0) ch->vy = -300;
        
        // Гравитация
        ch->vy += 50;
        ch->y += ch->vy;
        
        // Коллизия с полом (упрощённо)
        if (ch->y > WORLD_H * BLOCK_SIZE - BLOCK_SIZE) {
            ch->y = WORLD_H * BLOCK_SIZE - BLOCK_SIZE;
            ch->vy = 0;
        }
    }
    
    // Обработать события
    server_process_events(srv);
    
    srv->current_tick++;
    srv->stats.total_ticks++;
    
    // Вычислить время тика
    gettimeofday(&end, NULL);
    double tick_time = (end.tv_sec - start.tv_sec) * 1000.0 + 
                       (end.tv_usec - start.tv_usec) / 1000.0;
    
    // Скользящее среднее
    srv->stats.avg_tick_time_ms = srv->stats.avg_tick_time_ms * 0.9 + tick_time * 0.1;
}

// Отправка снапшотов
void server_send_snapshots(ServerState* srv) {
    WorldSnapshot snap = {0};
    snap.tick = srv->current_tick;
    snap.char_count = srv->char_count;
    
    for (int i = 0; i < srv->char_count; i++) {
        SnapshotChar* sc = &snap.chars[i];
        Character* ch = &srv->world.characters[i];
        sc->x = ch->x;
        sc->y = ch->y;
        sc->vx = ch->vx / 4;
        sc->vy = ch->vy / 4;
        sc->hp = ch->hp;
        sc->anim_state = ch->anim_state;
        sc->is_shield_active = ch->is_shield_active;
        sc->is_aiming = ch->is_aiming;
        sc->is_holding_flag = ch->is_holding_flag;
        sc->is_climbing = ch->is_climbing;
        sc->team = ch->team;
        sc->type = ch->type;
    }
    
    NetworkPacket pkt = {0};
    pkt.type = PKT_SNAPSHOT;
    memcpy(&pkt.snapshot, &snap, sizeof(WorldSnapshot));
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (srv->clients[i].active) {
            sendto(srv->sock, &pkt, sizeof(NetworkPacket), 0,
                   (struct sockaddr*)&srv->clients[i].addr, srv->clients[i].addr_len);
            srv->clients[i].packets_sent++;
            srv->stats.packets_sent++;
        }
    }
}

// Запуск сервера
void server_run(ServerState* srv) {
    struct timeval timeout = {0, 1000000 / SNAPSHOT_RATE};
    
    printf("\n=== Song of Stone Server ===\n");
    printf("Type 'help' for commands\n\n");
    
    while (srv->running) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(srv->sock, &read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        
        if (select(MAX(srv->sock, STDIN_FILENO) + 1, &read_fds, NULL, NULL, &timeout) > 0) {
            // Сокет с данными
            if (FD_ISSET(srv->sock, &read_fds)) {
                struct sockaddr_in client_addr;
                socklen_t addr_len = sizeof(client_addr);
                uint8_t buffer[1500];
                
                int len = recvfrom(srv->sock, buffer, sizeof(buffer), 0,
                                   (struct sockaddr*)&client_addr, &addr_len);
                if (len > 0) {
                    server_handle_packet(srv, buffer, len, &client_addr, addr_len);
                }
            }
            
            // Команды с клавиатуры
            if (FD_ISSET(STDIN_FILENO, &read_fds)) {
                char cmd[256];
                if (fgets(cmd, sizeof(cmd), stdin)) {
                    // Удалить newline
                    cmd[strcspn(cmd, "\n")] = 0;
                    
                    if (strlen(cmd) > 0) {
                        if (!server_handle_admin_command(srv, cmd)) {
                            printf("Unknown command: %s\n", cmd);
                            printf("Type 'help' for available commands\n");
                        }
                    }
                }
            }
        }
        
        server_update(srv);
        server_send_snapshots(srv);
        
        timeout.tv_usec = 1000000 / SNAPSHOT_RATE;
    }
    
    server_shutdown(srv);
}

// Завершение работы
void server_shutdown(ServerState* srv) {
    server_log(srv, LOG_INFO, "Server shutting down...");
    server_print_stats(srv);
    
    // Сохранить логи
    server_save_logs(srv, "server_log_final.txt");
    
    close(srv->sock);
    printf("Server stopped.\n");
}

// Главная функция
int main(void) {
    ServerState server;
    server_init(&server, NET_PORT);
    server_run(&server);
    return 0;
}
