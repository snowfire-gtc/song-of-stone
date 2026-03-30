#include "network.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>

// Глобальные переменные клиента
static int client_sock = -1;
static struct sockaddr_in server_addr;
static int my_player_id = -1;
static bool connected = false;
static PlayerState remote_players[MAX_PLAYERS];
static int remote_player_count = 0;
static int score_blue = 0;
static int score_red = 0;

// Подключение к серверу
int connect_to_server(const char* ip, int port) {
    client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sock < 0) {
        perror("Socket creation failed");
        return -1;
    }
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server_addr.sin_addr);
    
    if (connect(client_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(client_sock);
        return -1;
    }
    
    printf("Connected to server %s:%d\n", ip, port);
    connected = true;
    return 0;
}

// Отправка пакета
int send_packet(void* data, size_t size) {
    if (!connected || client_sock < 0) return -1;
    ssize_t sent = send(client_sock, data, size, 0);
    return (sent == (ssize_t)size) ? 0 : -1;
}

// Получение пакета
int recv_packet(void* buffer, size_t size) {
    if (!connected || client_sock < 0) return -1;
    ssize_t received = recv(client_sock, buffer, size, MSG_WAITALL);
    return (received == (ssize_t)size) ? 0 : -1;
}

// Запрос на подключение
int send_connect_request(const char* username, int team) {
    PacketConnect pkt = {0};
    pkt.header.type = PACKET_CONNECT;
    pkt.header.size = sizeof(PacketConnect) - sizeof(PacketHeader);
    strncpy(pkt.username, username, 31);
    pkt.team = team;
    
    if (send_packet(&pkt, sizeof(pkt)) < 0) return -1;
    
    // Ждём ответ
    PacketHeader hdr;
    if (recv_packet(&hdr, sizeof(hdr)) < 0) return -1;
    
    if (hdr.type == PACKET_CONNECT_ACK) {
        PacketConnectAck ack;
        memcpy(&ack.header, &hdr, sizeof(hdr));
        if (recv_packet(&ack.player_id, sizeof(ack.player_id)) < 0) return -1;
        my_player_id = ack.player_id;
        printf("Authenticated as player %d\n", my_player_id);
        return 0;
    }
    
    return -1;
}

// Отправка ввода
void send_player_input(PacketInput* input) {
    input->header.type = PACKET_INPUT;
    input->header.size = sizeof(PacketInput) - sizeof(PacketHeader);
    send_packet(input, sizeof(PacketInput));
}

// Получение состояния игры
bool receive_game_state(PlayerState* players, int* count, int* blue_score, int* red_score) {
    PacketHeader hdr;
    
    // Устанавливаем таймаут
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000; // 100ms
    setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    if (recv_packet(&hdr, sizeof(hdr)) < 0) return false;
    
    if (hdr.type == PACKET_GAME_STATE) {
        PacketGameState state;
        memcpy(&state.header, &hdr, sizeof(hdr));
        
        if (recv_packet(&state.player_count, sizeof(state.player_count)) < 0) return false;
        
        *count = state.player_count;
        for (int i = 0; i < state.player_count && i < MAX_PLAYERS; i++) {
            if (recv_packet(&players[i], sizeof(PlayerState)) < 0) return false;
        }
        
        if (recv_packet(blue_score, sizeof(int)) < 0) return false;
        if (recv_packet(red_score, sizeof(int)) < 0) return false;
        
        return true;
    }
    
    return false;
}

// Отключение
void disconnect_from_server(void) {
    if (client_sock >= 0) {
        close(client_sock);
        client_sock = -1;
    }
    connected = false;
    my_player_id = -1;
    printf("Disconnected from server\n");
}

// Остановка клиента
void stop_client(void) {
    disconnect_from_server();
    printf("Client stopped\n");
}

// Получение удалённых игроков
PlayerState* get_remote_players(int* count) {
    *count = remote_player_count;
    return remote_players;
}

// Standalone клиент
#ifdef STANDALONE_CLIENT
#include <raylib.h>

int main(void) {
    InitWindow(800, 600, "CTF Client");
    SetTargetFPS(60);
    
    const char* server_ip = "127.0.0.1";
    const char* username = "Player1";
    int team = 0;
    
    printf("Connecting to %s...\n", server_ip);
    
    if (connect_to_server(server_ip, SERVER_PORT) < 0) {
        printf("Failed to connect.\n");
        CloseWindow();
        return 1;
    }
    
    if (send_connect_request(username, team) < 0) {
        printf("Failed to authenticate.\n");
        disconnect_from_server();
        CloseWindow();
        return 1;
    }
    
    printf("Connected as %s (ID: %d)\n", username, my_player_id);
    
    while (!WindowShouldClose()) {
        PacketInput input = {0};
        input.player_id = my_player_id;
        
        if (IsKeyDown(KEY_W)) input.move_y = -1;
        if (IsKeyDown(KEY_S)) input.move_y = 1;
        if (IsKeyDown(KEY_A)) input.move_x = -1;
        if (IsKeyDown(KEY_D)) input.move_x = 1;
        if (IsKeyPressed(KEY_SPACE)) input.jump = true;
        
        send_player_input(&input);
        
        int count = 0;
        int blue = 0, red = 0;
        if (receive_game_state(remote_players, &count, &blue, &red)) {
            remote_player_count = count;
            score_blue = blue;
            score_red = red;
            
            BeginDrawing();
            ClearBackground(RAYWHITE);
            
            for (int i = 0; i < count; i++) {
                PlayerState* p = &remote_players[i];
                Color col = (p->team == TEAM_BLUE) ? BLUE : RED;
                DrawRectangle(p->x * 2, p->y * 2, 16, 16, col);
                DrawText(p->username, p->x * 2, p->y * 2 - 10, 10, BLACK);
            }
            
            char score_text[64];
            snprintf(score_text, sizeof(score_text), "Blue: %d | Red: %d", score_blue, score_red);
            DrawText(score_text, 10, 10, 20, BLACK);
            
            DrawFPS(700, 10);
            EndDrawing();
        }
    }
    
    stop_client();
    CloseWindow();
    return 0;
}
#endif
