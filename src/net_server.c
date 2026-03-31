// Net Server Implementation - Серверная часть сети
#include "net_server.h"
#include "net_protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

double get_time_seconds(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

static int set_nonblocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) return 0;
    return fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) != -1;
}

int net_server_init(GameServer* server, ServerConfig* config, World* world) {
    memset(server, 0, sizeof(GameServer));
    server->config = *config;
    server->world = world;
    server->socket_fd = -1;
    server->running = 1;
    server->client_count = 0;
    server->game_time = 8 * 3600; // 8:00 утра
    server->weather = WEATHER_CLEAR;
    
    // Инициализация клиентов
    for (int i = 0; i < MAX_CLIENTS; i++) {
        server->clients[i].connected = 0;
        server->clients[i].is_bot = 0;
        server->characters[i].active = 0;
    }
    
    // Создание сокета
    server->socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (server->socket_fd < 0) {
        perror("Не удалось создать сокет");
        return 0;
    }
    
    // Разрешить повторное использование порта
    int reuse = 1;
    if (setsockopt(server->socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("setsockopt SO_REUSEADDR");
        close(server->socket_fd);
        return 0;
    }
    
    // Привязка к порту
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(server->config.port);
    
    if (bind(server->socket_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Не удалось привязаться к порту");
        close(server->socket_fd);
        return 0;
    }
    
    // Установка в неблокирующий режим
    if (!set_nonblocking(server->socket_fd)) {
        perror("Не удалось установить неблокирующий режим");
        close(server->socket_fd);
        return 0;
    }
    
    printf("Сервер инициализирован на порту %d\n", server->config.port);
    return 1;
}

void net_server_shutdown(GameServer* server) {
    if (!server) return;
    
    server->running = 0;
    
    // Отключение всех клиентов
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server->clients[i].connected) {
            net_server_disconnect_client(server, i, "Server shutdown");
        }
    }
    
    // Закрытие сокета
    if (server->socket_fd >= 0) {
        close(server->socket_fd);
        server->socket_fd = -1;
    }
    
    printf("Сервер остановлен\n");
}

int net_server_accept_client(GameServer* server) {
    if (server->client_count >= server->config.max_clients) {
        return -1; // Сервер полон
    }
    
    // Найти свободный слот
    int free_slot = -1;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!server->clients[i].connected) {
            free_slot = i;
            break;
        }
    }
    
    if (free_slot < 0) return -1;
    
    // Приём нового соединения (для UDP просто запоминаем адрес первого пакета)
    // В UDP нет понятия "соединение", поэтому клиент считается подключённым после первого валидного пакета
    
    return free_slot;
}

void net_server_disconnect_client(GameServer* server, int client_id, const char* reason) {
    if (client_id < 0 || client_id >= MAX_CLIENTS) return;
    if (!server->clients[client_id].connected) return;
    
    printf("Клиент %d отключён: %s\n", client_id, reason ? reason : "неизвестно");
    
    // Отправить уведомление об отключении
    PacketHeader header;
    init_packet_header(&header, PKT_DISCONNECT, 0, 0);
    
    char msg[256] = {0};
    if (reason) strncpy(msg, reason, sizeof(msg) - 1);
    
    uint8_t buffer[512];
    size_t size = serialize_packet(&header, (uint8_t*)msg, strlen(msg), buffer, sizeof(buffer));
    
    if (size > 0) {
        sendto(server->socket_fd, buffer, size, 0,
               (struct sockaddr*)&server->clients[client_id].address,
               server->clients[client_id].addr_len);
    }
    
    // Очистка слота
    server->clients[client_id].connected = 0;
    server->clients[client_id].is_bot = 0;
    server->characters[client_id].active = 0;
    server->client_count--;
    
    // Уведомить остальных игроков
    char chat_msg[512];
    snprintf(chat_msg, sizeof(chat_msg), "%s покинул игру", 
             server->clients[client_id].name);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (i != client_id && server->clients[i].connected) {
            net_server_send_chat(server, i, chat_msg);
        }
    }
}

void net_server_send_to_client(GameServer* server, int client_id, const void* data, size_t size) {
    if (client_id < 0 || client_id >= MAX_CLIENTS) return;
    if (!server->clients[client_id].connected) return;
    if (!data || size == 0) return;
    
    ssize_t sent = sendto(server->socket_fd, data, size, 0,
                          (struct sockaddr*)&server->clients[client_id].address,
                          server->clients[client_id].addr_len);
    
    if (sent < 0) {
        perror("Ошибка отправки пакета");
    }
}

void net_server_broadcast(GameServer* server, int exclude_id, const void* data, size_t size) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (i != exclude_id && server->clients[i].connected) {
            net_server_send_to_client(server, i, data, size);
        }
    }
}

void net_server_send_chat(GameServer* server, int client_id, const char* message) {
    if (!message) return;
    
    PacketHeader header;
    init_packet_header(&header, PKT_CHAT, 0, 0);
    
    uint8_t buffer[512];
    size_t size = serialize_packet(&header, (const uint8_t*)message, strlen(message), buffer, sizeof(buffer));
    
    if (size > 0 && client_id >= 0) {
        net_server_send_to_client(server, client_id, buffer, size);
    } else if (size > 0) {
        // Broadcast всем
        net_server_broadcast(server, -1, buffer, size);
    }
}

void net_server_send_snapshot(GameServer* server, int client_id) {
    if (client_id < 0 || client_id >= MAX_CLIENTS) return;
    if (!server->clients[client_id].connected) return;
    
    // Формирование снапшота состояния
    PacketSnapshot snapshot = {0};
    snapshot.timestamp = (uint32_t)(server->game_time * 1000);
    snapshot.weather = server->weather;
    snapshot.character_count = 0;
    
    // Добавить активных персонажей
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server->characters[i].active) {
            if (snapshot.character_count < MAX_CHARACTERS) {
                encode_character_delta(&server->characters[i], NULL, &snapshot.characters[snapshot.character_count]);
                snapshot.character_count++;
            }
        }
    }
    
    // Сериализация и отправка
    uint8_t buffer[2048];
    PacketHeader header;
    init_packet_header(&header, PKT_SNAPSHOT, 0, server->total_ticks);
    
    size_t payload_size = sizeof(PacketSnapshot);
    size_t total_size = serialize_packet(&header, (uint8_t*)&snapshot, payload_size, buffer, sizeof(buffer));
    
    if (total_size > 0) {
        net_server_send_to_client(server, client_id, buffer, total_size);
    }
}

void net_server_send_world_state(GameServer* server, int client_id) {
    if (client_id < 0 || client_id >= MAX_CLIENTS) return;
    if (!server->clients[client_id].connected) return;
    if (!server->world) return;
    
    // Отправка изменённых блоков (RLE сжатие)
    PacketBlockChange block_pkt = {0};
    
    // Для простоты отправляем все блоки (в реальной игре только изменения)
    block_pkt.x = 0;
    block_pkt.y = 0;
    block_pkt.count = WORLD_WIDTH * WORLD_HEIGHT;
    
    uint8_t buffer[8192];
    PacketHeader header;
    init_packet_header(&header, PKT_WORLD_STATE, 0, server->total_ticks);
    
    size_t compressed_size = compress_block_changes(server->world->blocks, block_pkt.count, 
                                                     buffer + sizeof(PacketHeader), 
                                                     sizeof(buffer) - sizeof(PacketHeader));
    
    if (compressed_size > 0) {
        size_t total_size = sizeof(PacketHeader) + compressed_size;
        net_server_send_to_client(server, client_id, buffer, total_size);
    }
}

void net_server_process_packet(GameServer* server, int client_id, PacketHeader* header, 
                               uint8_t* payload, size_t payload_size) {
    if (!server || !header) return;
    
    ClientInfo* client = &server->clients[client_id];
    
    // Проверка контрольной суммы
    if (!verify_packet(header, payload, payload_size)) {
        printf("Клиент %d: ошибка контрольной суммы\n", client_id);
        return;
    }
    
    // Rate limiting
    double now = get_time_seconds();
    if (now - client->last_pps_reset >= 1.0) {
        client->packets_per_second = 0;
        client->last_pps_reset = now;
    }
    client->packets_per_second++;
    
    if (client->packets_per_second > 60) {
        printf("Клиент %d: превышение лимита пакетов (%d/сек)\n", client_id, client->packets_per_second);
        return;
    }
    
    // Обработка по типу пакета
    switch (header->type) {
        case PKT_HELLO: {
            // Клиент приветствует сервер
            char name[32] = {0};
            if (payload_size > 0) {
                strncpy(name, (char*)payload, sizeof(name) - 1);
            }
            
            strncpy(client->name, name, sizeof(client->name) - 1);
            client->connected = 1;
            server->client_count++;
            
            printf("Клиент %d подключился: %s\n", client_id, client->name);
            
            // Отправить приветствие
            net_server_send_chat(server, client_id, "Добро пожаловать на сервер!");
            
            // Отправить состояние мира
            net_server_send_world_state(server, client_id);
            
            // Уведомить остальных
            char msg[256];
            snprintf(msg, sizeof(msg), "%s присоединился к игре", client->name);
            net_server_broadcast(server, client_id, msg, strlen(msg));
            break;
        }
        
        case PKT_INPUT: {
            // Обработка ввода от клиента
            PacketInput input = {0};
            if (deserialize_input(payload, payload_size, &input)) {
                // Применить ввод к персонажу клиента
                Character* ch = &server->characters[client_id];
                if (ch->active) {
                    ch->input_left = input.left;
                    ch->input_right = input.right;
                    ch->input_jump = input.jump;
                    ch->input_action = input.action;
                    ch->input_secondary = input.secondary;
                }
            }
            break;
        }
        
        case PKT_ACTION: {
            // Действие (атака, строительство, etc.)
            PacketAction action = {0};
            if (deserialize_action(payload, payload_size, &action)) {
                // TODO: Обработать действие на сервере
                printf("Клиент %d: действие типа %d\n", client_id, action.action_type);
            }
            break;
        }
        
        case PKT_CHAT: {
            // Чат
            if (payload_size > 0 && payload_size < 256) {
                char msg[512];
                snprintf(msg, sizeof(msg), "[%s]: %s", client->name, payload);
                printf("%s\n", msg);
                
                // Расслать всем
                net_server_broadcast(server, -1, msg, strlen(msg));
            }
            break;
        }
        
        case PKT_DISCONNECT: {
            // Клиент отключается
            net_server_disconnect_client(server, client_id, "Client requested disconnect");
            break;
        }
        
        default:
            printf("Клиент %d: неизвестный тип пакета %d\n", client_id, header->type);
            break;
    }
}

void net_server_update(GameServer* server, double dt) {
    if (!server || !server->running) return;
    
    // Обновление времени игры
    server->game_time += dt;
    
    // Приём пакетов от клиентов
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);
    
    ssize_t recv_size = recvfrom(server->socket_fd, server->recv_buffer, sizeof(server->recv_buffer), 0,
                                  (struct sockaddr*)&from_addr, &from_len);
    
    if (recv_size > (ssize_t)sizeof(PacketHeader)) {
        // Найти или создать клиента по адресу
        int client_id = -1;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (server->clients[i].connected &&
                server->clients[i].address.sin_addr.s_addr == from_addr.sin_addr.s_addr &&
                server->clients[i].address.sin_port == from_addr.sin_port) {
                client_id = i;
                break;
            }
        }
        
        // Если новый клиент, попробовать выделить слот
        if (client_id < 0) {
            client_id = net_server_accept_client(server);
            if (client_id >= 0) {
                server->clients[client_id].address = from_addr;
                server->clients[client_id].addr_len = from_len;
                server->clients[client_id].last_packet_time = get_time_seconds();
            }
        }
        
        if (client_id >= 0) {
            // Разбор пакета
            PacketHeader header;
            uint8_t payload[4096];
            size_t payload_size;
            
            if (deserialize_packet(server->recv_buffer, recv_size, &header, payload, sizeof(payload), &payload_size)) {
                server->clients[client_id].last_sequence = header.sequence;
                net_server_process_packet(server, client_id, &header, payload, payload_size);
            }
        }
    }
    
    // Отправка снапшотов клиентам (30 Гц)
    static double snapshot_timer = 0.0;
    snapshot_timer += dt;
    if (snapshot_timer >= 1.0 / 30.0) {
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (server->clients[i].connected) {
                net_server_send_snapshot(server, i);
            }
        }
        snapshot_timer = 0.0;
    }
}
