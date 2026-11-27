// client.c - Клиентская часть сети
#include "client.h"
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

static double get_time_seconds(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

static int set_nonblocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) return 0;
    return fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) != -1;
}

// Инициализация клиента
int client_init(GameClient* client, const char* server_ip, int port) {
    memset(client, 0, sizeof(GameClient));
    
    // Создание сокета
    client->socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (client->socket_fd < 0) {
        perror("Не удалось создать сокет");
        return 0;
    }
    
    // Установка в неблокирующий режим
    if (!set_nonblocking(client->socket_fd)) {
        perror("Не удалось установить неблокирующий режим");
        close(client->socket_fd);
        return 0;
    }
    
    // Настройка адреса сервера
    memset(&client->server_addr, 0, sizeof(client->server_addr));
    client->server_addr.sin_family = AF_INET;
    client->server_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, server_ip, &client->server_addr.sin_addr) <= 0) {
        perror("Неверный IP адрес сервера");
        close(client->socket_fd);
        return 0;
    }
    
    client->addr_len = sizeof(client->server_addr);
    client->connected = 0;
    client->player_id = -1;
    client->interpolation_delay = INTERPOLATION_DELAY;
    
    // Отправка приветствия
    PacketHeader header;
    init_packet_header(&header, PKT_HELLO, 0, 0);
    
    char hello_data[64] = {0};
    snprintf(hello_data, sizeof(hello_data), "Player");
    
    uint8_t buffer[512];
    size_t size = serialize_packet(&header, (uint8_t*)hello_data, strlen(hello_data), 
                                    buffer, sizeof(buffer));
    
    if (size > 0) {
        sendto(client->socket_fd, buffer, size, 0,
               (struct sockaddr*)&client->server_addr, client->addr_len);
        client->packets_sent++;
    }
    
    printf("Подключение к серверу %s:%d...\n", server_ip, port);
    return 1;
}

void client_shutdown(GameClient* client) {
    if (!client) return;
    
    // Отправить уведомление об отключении
    if (client->connected && client->socket_fd >= 0) {
        PacketHeader header;
        init_packet_header(&header, PKT_DISCONNECT, 0, 0);
        
        uint8_t buffer[256];
        size_t size = serialize_packet(&header, NULL, 0, buffer, sizeof(buffer));
        
        if (size > 0) {
            sendto(client->socket_fd, buffer, size, 0,
                   (struct sockaddr*)&client->server_addr, client->addr_len);
        }
    }
    
    if (client->socket_fd >= 0) {
        close(client->socket_fd);
        client->socket_fd = -1;
    }
    
    client->connected = 0;
    printf("Клиент отключён\n");
}

// Проверка подключения
int client_is_connected(GameClient* client) {
    return client && client->connected;
}

// Получение задержки
float client_get_latency(GameClient* client) {
    if (!client) return 0.0f;
    return client->avg_latency;
}

// Статус подключения
const char* client_get_connection_status(GameClient* client) {
    if (!client) return "Ошибка";
    if (!client->connected) return "Подключение...";
    if (client->avg_latency < 50) return "Отлично";
    if (client->avg_latency < 100) return "Хорошо";
    if (client->avg_latency < 200) return "Посредственно";
    return "Плохо";
}

// Отправка ввода
void client_send_input(GameClient* client, PlayerInput* input) {
    if (!client || !client->connected) return;
    
    PacketHeader header;
    init_packet_header(&header, PKT_INPUT, 0, client->packets_sent);
    
    // Convert PlayerInput to PacketInput for serialization
    PacketInput pkt_input = {0};
    pkt_input.left = input->move_left;
    pkt_input.right = input->move_right;
    pkt_input.jump = input->jump;
    pkt_input.action = input->attack;
    pkt_input.secondary = input->build;
    
    uint8_t payload[64];
    size_t payload_size = serialize_input(&pkt_input, payload, sizeof(payload));
    
    uint8_t buffer[512];
    size_t size = serialize_packet(&header, payload, payload_size, buffer, sizeof(buffer));
    
    if (size > 0) {
        sendto(client->socket_fd, buffer, size, 0,
               (struct sockaddr*)&client->server_addr, client->addr_len);
        client->packets_sent++;
        client->last_send_time = get_time_seconds();
    }
}

// Отправка действия
void client_send_action(GameClient* client, ActionType action, int target_x, int target_y) {
    if (!client || !client->connected) return;
    
    PacketHeader header;
    init_packet_header(&header, PKT_ACTION, 0, client->packets_sent);
    
    PacketAction pkt_action = {0};
    pkt_action.action_type = action;
    pkt_action.x = target_x;
    pkt_action.y = target_y;
    
    uint8_t payload[64];
    size_t payload_size = serialize_action(&pkt_action, payload, sizeof(payload));
    
    uint8_t buffer[512];
    size_t size = serialize_packet(&header, payload, payload_size, buffer, sizeof(buffer));
    
    if (size > 0) {
        sendto(client->socket_fd, buffer, size, 0,
               (struct sockaddr*)&client->server_addr, client->addr_len);
        client->packets_sent++;
    }
}

// Отправка сообщения в чат
void client_send_chat(GameClient* client, const char* message) {
    if (!client || !message) return;
    
    PacketHeader header;
    init_packet_header(&header, PKT_CHAT, 0, client->packets_sent);
    
    uint8_t buffer[512];
    size_t size = serialize_packet(&header, (const uint8_t*)message, strlen(message), 
                                    buffer, sizeof(buffer));
    
    if (size > 0) {
        sendto(client->socket_fd, buffer, size, 0,
               (struct sockaddr*)&client->server_addr, client->addr_len);
        client->packets_sent++;
    }
}

// Получение пакетов
int client_receive_packets(GameClient* client) {
    if (!client || client->socket_fd < 0) return 0;
    
    int packets_processed = 0;
    
    while (1) {
        ssize_t recv_size = recvfrom(client->socket_fd, client->recv_buffer, 
                                      sizeof(client->recv_buffer), 0,
                                      (struct sockaddr*)&client->server_addr, 
                                      &client->addr_len);
        
        if (recv_size < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break; // Нет больше данных
            }
            perror("Ошибка получения пакета");
            break;
        }
        
        client->packets_received++;
        client->last_recv_time = get_time_seconds();
        
        // Разбор пакета
        PacketHeader header;
        uint8_t payload[4096];
        size_t payload_size;
        
        if (!deserialize_packet(client->recv_buffer, recv_size, &header, 
                                payload, sizeof(payload), &payload_size)) {
            client->sequence_errors++;
            continue;
        }
        
        // Обработка по типу пакета
        switch (header.type) {
            case PKT_WELCOME: {
                // Сервер подтвердил подключение
                client->connected = 1;
                if (payload_size >= sizeof(uint16_t)) {
                    memcpy(&client->player_id, payload, sizeof(uint16_t));
                }
                
                if (payload_size > sizeof(uint16_t)) {
                    strncpy(client->server_name, (char*)payload + sizeof(uint16_t), sizeof(client->server_name) - 1);
                }
                
                printf("Подключён как игрок %d к серверу \"%s\"\n", 
                       client->player_id, client->server_name);
                break;
            }
            
            case PKT_SNAPSHOT: {
                // Снапшот состояния мира
                PacketSnapshot snapshot = {0};
                if (deserialize_snapshot(payload, payload_size, &snapshot)) {
                    client_apply_snapshot(client, &snapshot);
                }
                break;
            }
            
            case PKT_CHAT: {
                // Сообщение чата
                if (payload_size > 0 && payload_size < 256) {
                    printf("[CHAT] %.*s\n", (int)payload_size, payload);
                }
                break;
            }
            
            case PKT_DISCONNECT: {
                // Сервер отключает клиента
                printf("Сервер отключил клиента: %.*s\n", (int)payload_size, payload);
                client->connected = 0;
                break;
            }
            
            default:
                printf("Неизвестный тип пакета: %d\n", header.type);
                break;
        }
        
        packets_processed++;
    }
    
    return packets_processed;
}

// Применение снапшота
void client_apply_snapshot(GameClient* client, PacketSnapshot* snapshot) {
    if (!client || !snapshot) return;

    // Проверка порядка снапшотов
    if (snapshot->timestamp <= client->last_snapshot_tick) {
        return; // Устаревший снапшот
    }

    client->pending_snapshot_tick = snapshot->timestamp;

    // Сохраняем кейрфрейм для интерполяции (позиции на момент получения)
    SnapshotKeyframe* kf = &client->keyframes[client->keyframe_head];
    kf->tick = snapshot->timestamp;
    kf->recv_time = (float)get_time_seconds();
    kf->count = 0;

    // Обновление персонажей из снапшота
    for (int i = 0; i < snapshot->character_count && i < MAX_CHARACTERS; i++) {
        SnapshotChar* ec = &snapshot->characters[i];
        Character* ch = &client->local_world.characters[i];

        // Декодирование дельты
        decode_character_delta(ec, ch);
        if (ch->player_id == 0) ch->player_id = (int)ec->id;

        // Регистрируем целевую позицию в кейрфрейме
        if (kf->count < MAX_CHARACTERS) {
            kf->chars[kf->count].id = (int)ec->id;
            kf->chars[kf->count].x = (float)ec->x;
            kf->chars[kf->count].y = (float)ec->y;
            kf->count++;
        }
    }

    // Сдвигаем кольцевой буфер кейрфреймов
    client->keyframe_head = (client->keyframe_head + 1) % SNAPSHOT_BUFFER_SIZE;
    if (client->keyframe_count < SNAPSHOT_BUFFER_SIZE) {
        client->keyframe_count++;
    }

    // Обновление времени последнего снапшота
    client->last_snapshot_tick = snapshot->timestamp;
}

// Поиск двух соседних кейрфреймов, окружающих render_time
static bool client_find_keyframe_pair(GameClient* client, double render_time,
                                      SnapshotKeyframe** out_prev, SnapshotKeyframe** out_next) {
    SnapshotKeyframe* prev = NULL;
    SnapshotKeyframe* next = NULL;

    // Обходим кольцевой буфер от старых кейрфреймов к новым
    int start = (client->keyframe_head - client->keyframe_count + SNAPSHOT_BUFFER_SIZE * 2) % SNAPSHOT_BUFFER_SIZE;
    for (int i = 0; i < client->keyframe_count; i++) {
        SnapshotKeyframe* kf = &client->keyframes[(start + i) % SNAPSHOT_BUFFER_SIZE];
        if (kf->count == 0) continue;
        if ((double)kf->recv_time <= render_time) {
            prev = kf;
        } else {
            next = kf;
            break;
        }
    }

    if (!prev || !next) return false;
    *out_prev = prev;
    *out_next = next;
    return true;
}

// Интерполяция персонажей между снапшотами
// alpha >= 0 — явный коэффициент смешивания последних двух кейрфреймов;
// alpha < 0 — автоматический расчёт по виртуальному времени рендера
// (now - interpolation_delay), как в классических сетевых интерполяторах.
void client_interpolate_characters(GameClient* client, double alpha) {
    if (!client || !client->connected) return;
    if (client->keyframe_count < 2) return;

    SnapshotKeyframe *kf_prev, *kf_next;

    if (alpha >= 0.0) {
        // Явный alpha: берём две последние записи буфера
        int start = (client->keyframe_head - client->keyframe_count + SNAPSHOT_BUFFER_SIZE * 2) % SNAPSHOT_BUFFER_SIZE;
        kf_prev = &client->keyframes[(start + client->keyframe_count - 2) % SNAPSHOT_BUFFER_SIZE];
        kf_next = &client->keyframes[(start + client->keyframe_count - 1) % SNAPSHOT_BUFFER_SIZE];
        if (kf_prev->count == 0 || kf_next->count == 0) return;
        if (alpha > 1.0) alpha = 1.0;
    } else {
        // Авто-подбор пары кейрфреймов по виртуальному времени рендера
        double now = get_time_seconds();
        double render_time = now - (double)client->interpolation_delay;
        if (!client_find_keyframe_pair(client, render_time, &kf_prev, &kf_next)) return;
        double span = (double)kf_next->recv_time - (double)kf_prev->recv_time;
        if (span <= 0.0) return;
        alpha = (render_time - (double)kf_prev->recv_time) / span;
        if (alpha < 0.0) alpha = 0.0;
        if (alpha > 1.0) alpha = 1.0;
    }

    // Линейная интерполяция позиций каждого персонажа
    for (int i = 0; i < kf_next->count; i++) {
        int id = kf_next->chars[i].id;
        float tx = kf_next->chars[i].x;
        float ty = kf_next->chars[i].y;

        // Ищем того же персонажа в предыдущем кейрфрейме
        bool found_prev = false;
        float px = tx, py = ty;
        for (int j = 0; j < kf_prev->count; j++) {
            if (kf_prev->chars[j].id == id) {
                px = kf_prev->chars[j].x;
                py = kf_prev->chars[j].y;
                found_prev = true;
                break;
            }
        }
        if (!found_prev) continue; // новый персонаж — без интерполяции

        // Применяем результат к локальному миру
        for (int k = 0; k < client->local_world.char_count && k < MAX_CHARACTERS; k++) {
            Character* ch = &client->local_world.characters[k];
            if ((int)ch->player_id == id) {
                ch->x = (int)(px + (tx - px) * (float)alpha);
                ch->y = (int)(py + (ty - py) * (float)alpha);
                break;
            }
        }
    }
}

// Основное обновление клиента
void client_update(GameClient* client, double dt) {
    if (!client) return;
    
    // Получение пакетов от сервера
    client_receive_packets(client);
    
    // Таймаут подключения
    if (!client->connected) {
        client->reconnect_timer += dt;
        if (client->reconnect_timer > 5.0) {
            printf("Таймаут подключения к серверу\n");
            client->reconnect_timer = 0;
        }
    }
    
    // Расчёт средней задержки (упрощённо)
    if (client->last_send_time > 0 && client->last_recv_time > 0) {
        float latency = (client->last_recv_time - client->last_send_time) * 1000.0f;
        client->avg_latency = client->avg_latency * 0.9f + latency * 0.1f;
    }

    // Интерполяция позиций удалённых персонажей (авто-режим по времени)
    client_interpolate_characters(client, -1.0);
}
