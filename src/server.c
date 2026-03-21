// server_main.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "../shared/common_game.h"
#include "../shared/net_protocol.h"

#define MAX_CLIENTS 128

typedef struct {
    int active;
    struct sockaddr_in addr;
    socklen_t addr_len;
    uint16_t player_id;
    PlayerInput last_input;
    uint32_t last_input_tick;
} Client;

static int server_sock;
static Client clients[MAX_CLIENTS];
static WorldState world;
static uint32_t current_tick = 0;

// Инициализация сервера
void server_init(void) {
    server_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_sock < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(NET_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(1);
    }

    world = gen_world_default(); // из gen.c
    printf("Server started on port %d\n", NET_PORT);
}

// Обработка входящего пакета
void handle_packet(uint8_t* data, int len, struct sockaddr_in* addr, socklen_t addr_len) {
    if (len < 1) return;
    NetworkPacket* pkt = (NetworkPacket*)data;

    if (pkt->type == PKT_HELLO) {
        // Найти свободного клиента
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (!clients[i].active) {
                clients[i].active = 1;
                clients[i].addr = *addr;
                clients[i].addr_len = addr_len;
                clients[i].player_id = i;
                pkt->player_id = i;
                strncpy(world.characters[i].name, pkt->hello_name, MAX_NAME_LEN);
                world.characters[i].player_id = i;
                world.characters[i].team = (i % 2 == 0) ? TEAM_BLUE : TEAM_RED;
                world.characters[i].hp = PLAYER_MAX_HP;
                world.characters[i].x = (world.throne_blue.x + world.throne_red.x) / 2;
                world.characters[i].y = world.throne_blue.y;
                world.char_count = i + 1;

                // Отправить подтверждение
                sendto(server_sock, pkt, sizeof(NetworkPacket), 0,
                       (struct sockaddr*)addr, addr_len);
                printf("Player %s joined as ID %d\n", pkt->hello_name, i);
                break;
            }
        }
    }
    else if (pkt->type == PKT_INPUT) {
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].active && clients[i].player_id == pkt->player_id) {
                clients[i].last_input = pkt->input;
                clients[i].last_input_tick = current_tick;
                break;
            }
        }
    }
}

// Обновление игры на сервере
void server_update(void) {
    // Применить ввод
    for (int i = 0; i < world.char_count; i++) {
        Character* ch = &world.characters[i];
        if (ch->hp <= 0) continue;

        Client* client = &clients[ch->player_id];
        PlayerInput* input = &client->last_input;

        // Простой пример движения
        if (input->move_left) ch->x -= 2;
        if (input->move_right) ch->x += 2;
        if (input->jump && ch->vy == 0) ch->vy = -300;

        // Здесь — полная логика из logic_update()
        // (в реальности вызываем logic_update_characters(&world, current_tick))
    }

    current_tick++;
}

// Отправка снапшотов
void server_send_snapshots(void) {
    WorldSnapshot snap = {0};
    snap.tick = current_tick;
    snap.char_count = world.char_count;

    for (int i = 0; i < world.char_count; i++) {
        SnapshotChar* sc = &snap.chars[i];
        Character* ch = &world.characters[i];
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
        if (clients[i].active) {
            sendto(server_sock, &pkt, sizeof(NetworkPacket), 0,
                   (struct sockaddr*)&clients[i].addr, clients[i].addr_len);
        }
    }
}

// Основной цикл
int main(void) {
    server_init();
    struct timeval timeout = {0, 1000000 / SNAPSHOT_RATE}; // 30 FPS

    while (1) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server_sock, &read_fds);

        if (select(server_sock + 1, &read_fds, NULL, NULL, &timeout) > 0) {
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            uint8_t buffer[1500];

            int len = recvfrom(server_sock, buffer, sizeof(buffer), 0,
                               (struct sockaddr*)&client_addr, &addr_len);
            if (len > 0) {
                handle_packet(buffer, len, &client_addr, addr_len);
            }
        }

        server_update();
        server_send_snapshots();
        timeout.tv_usec = 1000000 / SNAPSHOT_RATE; // сброс таймаута
    }

    close(server_sock);
    return 0;
}
