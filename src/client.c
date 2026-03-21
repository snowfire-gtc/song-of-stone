// client_main.c (упрощённый main)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "../shared/common_game.h"
#include "../shared/net_protocol.h"
#include "raylib.h"

static int client_sock;
static struct sockaddr_in server_addr;
static WorldState local_world;
static uint32_t last_snapshot_tick = 0;

// Отправка ввода
void client_send_input(PlayerInput input) {
    NetworkPacket pkt = {0};
    pkt.type = PKT_INPUT;
    pkt.player_id = local_world.local_player_id;
    pkt.input = input;
    sendto(client_sock, &pkt, sizeof(pkt), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
}

// Обработка снапшота
void client_apply_snapshot(WorldSnapshot* snap) {
    if (snap->tick <= last_snapshot_tick) return;
    last_snapshot_tick = snap->tick;

    for (int i = 0; i < snap->char_count; i++) {
        SnapshotChar* sc = &snap->chars[i];
        Character* ch = &local_world.characters[i];
        // Интерполяция или немедленное обновление
        ch->x = sc->x;
        ch->y = sc->y;
        ch->vx = sc->vx * 4;
        ch->vy = sc->vy * 4;
        ch->hp = sc->hp;
        ch->anim_state = sc->anim_state;
        ch->is_shield_active = sc->is_shield_active;
        ch->is_aiming = sc->is_aiming;
        ch->is_holding_flag = sc->is_holding_flag;
        ch->is_climbing = sc->is_climbing;
        ch->team = sc->team;
        ch->type = sc->type;
    }
}

// Подключение
void client_connect(const char* ip) {
    client_sock = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(NET_PORT);
    inet_pton(AF_INET, ip, &server_addr.sin_addr);

    // Отправить HELLO
    NetworkPacket hello = {0};
    hello.type = PKT_HELLO;
    strcpy(hello.hello_name, "Player");

    sendto(client_sock, &hello, sizeof(hello), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));

    // Получить ответ с player_id
    NetworkPacket ack;
    socklen_t addr_len = sizeof(server_addr);
    recvfrom(client_sock, &ack, sizeof(ack), 0, (struct sockaddr*)&server_addr, &addr_len);
    local_world.local_player_id = ack.player_id;
    printf("Connected as player %d\n", ack.player_id);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <server_ip>\n", argv[0]);
        return 1;
    }

    client_connect(argv[1]);
    InitWindow(1280, 720, "Medieval CTF - Client");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        // Сбор ввода
        PlayerInput input = {0};
        input.move_left = IsKeyDown(KEY_A);
        input.move_right = IsKeyDown(KEY_D);
        input.jump = IsKeyDown(KEY_SPACE);
        input.attack = IsKeyDown(KEY_F);
        input.aim = IsKeyDown(KEY_R);
        input.dig = IsKeyDown(KEY_E);
        input.shield = IsKeyDown(KEY_LEFT_CONTROL);

        client_send_input(input);

        // Получение снапшотов
        NetworkPacket pkt;
        socklen_t addr_len = sizeof(server_addr);
        int len = recvfrom(client_sock, &pkt, sizeof(pkt), MSG_DONTWAIT,
                           (struct sockaddr*)&server_addr, &addr_len);
        if (len > 0 && pkt.type == PKT_SNAPSHOT) {
            client_apply_snapshot(&pkt.snapshot);
        }

        // Отрисовка
        BeginDrawing();
        ClearBackground(RAYWHITE);
        draw_all(&local_world); // твои функции отрисовки
        EndDrawing();
    }

    CloseWindow();
    close(client_sock);
    return 0;
}
