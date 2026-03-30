#include "network.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>

// Глобальные переменные клиента
static int client_sock = -1;
static struct sockaddr_in server_addr;
static int my_player_id = -1;
static bool connected = false;
static PlayerState remote_players[MAX_PLAYERS];
static int remote_player_count = 0;
static int score_blue = 0;
static int score_red = 0;
static int flag_progress = 0;
static pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;

// Вспомогательная функция для неблокирующего сокета
int set_nonblocking_client(int sock) {
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(sock, F_SETFL, flags | O_NONBLOCK);
}

// Подключение к серверу
int connect_to_server(const char* ip, int port) {
    client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sock < 0) {
        perror("Socket creation failed");
        return -1;
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        close(client_sock);
        return -1;
    }
    
    if (connect(client_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(client_sock);
        return -1;
    }
    
    if (set_nonblocking_client(client_sock) < 0) {
        perror("Set non-blocking failed");
        close(client_sock);
        return -1;
    }
    
    printf("Connected to server %s:%d\n", ip, port);
    connected = true;
    return 0;
}

// Отправка пакета подключения
int send_connect_request(const char* username, int team) {
    if (!connected) return -1;
    
    PacketConnect pkt = {0};
    pkt.type = MSG_CONNECT_REQ;
    strncpy(pkt.username, username, 31);
    pkt.team = team;
    
    if (send(client_sock, &pkt, sizeof(pkt), 0) < 0) {
        perror("Send failed");
        return -1;
    }
    
    // Ждем ответ
    usleep(100000); // 100ms
    
    PacketConnectResp resp;
    int bytes = recv(client_sock, &resp, sizeof(resp), 0);
    
    if (bytes > 0 && resp.type == MSG_CONNECT_RESP && resp.success) {
        my_player_id = resp.player_id;
        printf("Connected as player ID %d (%s)\n", my_player_id, username);
        return 0;
    } else {
        printf("Connection rejected by server\n");
        return -1;
    }
}

// Отправка ввода
int send_input(uint8_t keys, float mouse_x, float mouse_y, 
               uint8_t action_type, uint8_t block_to_place) {
    if (!connected || my_player_id < 0) return -1;
    
    PacketInput pkt = {0};
    pkt.type = MSG_PLAYER_INPUT;
    pkt.player_id = my_player_id;
    pkt.keys = keys;
    pkt.mouse_x = mouse_x;
    pkt.mouse_y = mouse_y;
    pkt.action_type = action_type;
    pkt.block_to_place = block_to_place;
    
    return send(client_sock, &pkt, sizeof(pkt), 0);
}

// Получение состояния игры
int receive_game_state() {
    if (!connected) return -1;
    
    PacketGameState state;
    int bytes = recv(client_sock, &state, sizeof(state), 0);
    
    if (bytes > 0 && state.type == MSG_GAME_STATE) {
        pthread_mutex_lock(&client_mutex);
        
        score_blue = state.score_blue;
        score_red = state.score_red;
        flag_progress = state.flag_progress;
        remote_player_count = state.active_players;
        
        for (int i = 0; i < remote_player_count; i++) {
            remote_players[i] = state.players[i];
        }
        
        pthread_mutex_unlock(&client_mutex);
        return 0;
    }
    
    return -1;
}

// Получение счета
void get_scores(int* blue, int* red) {
    *blue = score_blue;
    *red = score_red;
}

// Получение прогресса флага
int get_flag_progress() {
    return flag_progress;
}

// Получение состояния удаленных игроков
PlayerState* get_remote_players(int* count) {
    *count = remote_player_count;
    return remote_players;
}

// Мой ID игрока
int get_my_player_id() {
    return my_player_id;
}

// Проверка подключения
bool is_connected() {
    return connected;
}

// Отключение
void disconnect_from_server() {
    if (client_sock >= 0) {
        close(client_sock);
        client_sock = -1;
    }
    connected = false;
    my_player_id = -1;
    printf("Disconnected from server\n");
}

// Пример использования в игровом цикле
#ifdef STANDALONE_CLIENT
#include <raylib.h>

int main() {
    // Инициализация окна
    InitWindow(800, 600, "CTF Client");
    SetTargetFPS(60);
    
    const char* server_ip = "127.0.0.1";
    const char* username = "Player1";
    int team = 0; // Blue
    
    printf("Connecting to %s...\n", server_ip);
    
    if (connect_to_server(server_ip, SERVER_PORT) < 0) {
        printf("Failed to connect. Make sure server is running.\n");
        CloseWindow();
        return 1;
    }
    
    if (send_connect_request(username, team) < 0) {
        printf("Failed to authenticate.\n");
        disconnect_from_server();
        CloseWindow();
        return 1;
    }
    
    // Игровой цикл
    while (!WindowShouldClose()) {
        // Обработка ввода
        uint8_t keys = 0;
        if (IsKeyDown(KEY_LEFT)) keys |= 1;
        if (IsKeyDown(KEY_RIGHT)) keys |= 2;
        if (IsKeyDown(KEY_UP)) keys |= 4;
        if (IsKeyDown(KEY_DOWN)) keys |= 8;
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) keys |= 16;
        
        Vector2 mouse = GetMousePosition();
        
        // Отправка ввода на сервер
        send_input(keys, mouse.x, mouse.y, 0, 0);
        
        // Получение состояния
        receive_game_state();
        
        // Отрисовка
        BeginDrawing();
        ClearBackground(RAYWHITE);
        
        // Отображение информации
        DrawText(TextFormat("Connected as: %s (ID: %d)", username, my_player_id), 10, 10, 20, DARKGREEN);
        DrawText(TextFormat("Score - Blue: %d | Red: %d", score_blue, score_red), 10, 40, 20, BLACK);
        DrawText(TextFormat("Flag Progress: %d%%", flag_progress), 10, 70, 20, ORANGE);
        
        // Отрисовка других игроков
        int count;
        PlayerState* players = get_remote_players(&count);
        for (int i = 0; i < count; i++) {
            if (players[i].id != my_player_id) {
                Color c = (players[i].team == 0) ? BLUE : RED;
                DrawCircle(players[i].x * 20, players[i].y * 20, 15, c);
                DrawText(players[i].username, players[i].x * 20 - 20, players[i].y * 20 - 30, 10, BLACK);
            }
        }
        
        DrawFPS(700, 10);
        EndDrawing();
    }
    
    disconnect_from_server();
    CloseWindow();
    return 0;
}
#endif
