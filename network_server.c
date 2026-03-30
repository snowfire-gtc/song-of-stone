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

// Глобальные переменные сервера
static int server_fd = -1;
static struct sockaddr_in server_addr;
static PlayerState players[MAX_PLAYERS];
static int player_count = 0;
static int scores[2] = {0, 0}; // Blue, Red
static pthread_mutex_t game_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool server_running = false;

// Вспомогательная функция для неблокирующего сокета
int set_nonblocking(int sock) {
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(sock, F_SETFL, flags | O_NONBLOCK);
}

// Отправка пакета
int send_packet(int sock, void* data, size_t size) {
    return send(sock, data, size, 0);
}

// Получение пакета
int recv_packet(int sock, void* buffer, size_t size) {
    return recv(sock, buffer, size, 0);
}

// Обработка подключения нового игрока
void handle_connect_req(int client_sock, PacketConnect* pkt, struct sockaddr_in* client_addr) {
    pthread_mutex_lock(&game_mutex);
    
    if (player_count >= MAX_PLAYERS) {
        PacketConnectResp resp = {0};
        resp.type = MSG_CONNECT_RESP;
        resp.player_id = -1;
        resp.success = 0;
        send_packet(client_sock, &resp, sizeof(resp));
        pthread_mutex_unlock(&game_mutex);
        close(client_sock);
        return;
    }
    
    // Создаем нового игрока
    int id = player_count++;
    PlayerState* p = &players[id];
    memset(p, 0, sizeof(PlayerState));
    p->id = id;
    strncpy(p->username, pkt->username, 31);
    p->team = pkt->team;
    p->x = (pkt->team == 0) ? 5.0f : WORLD_WIDTH - 5.0f; // Спавн у трона
    p->y = 10.0f;
    p->alive = true;
    p->skin_color = (pkt->team == 0) ? 0x0000FF : 0xFF0000; // Blue or Red
    
    printf("Player %s connected as ID %d (Team %s)\n", 
           pkt->username, id, (pkt->team == 0) ? "Blue" : "Red");
    
    PacketConnectResp resp = {0};
    resp.type = MSG_CONNECT_RESP;
    resp.player_id = id;
    resp.success = 1;
    send_packet(client_sock, &resp, sizeof(resp));
    
    pthread_mutex_unlock(&game_mutex);
}

// Обработка ввода от игрока
void handle_player_input(int client_sock, PacketInput* pkt) {
    pthread_mutex_lock(&game_mutex);
    
    if (pkt->player_id < 0 || pkt->player_id >= player_count) {
        pthread_mutex_unlock(&game_mutex);
        return;
    }
    
    PlayerState* p = &players[pkt->player_id];
    
    // Применяем ввод к физике игрока
    float speed = 0.2f;
    float jump_force = 0.5f;
    
    if (pkt->keys & 1) p->vx -= speed; // Left
    if (pkt->keys & 2) p->vx += speed; // Right
    if (pkt->keys & 4) { // Up/Jump
        // Проверка на земле ли игрок (упрощенно)
        if (p->vy == 0) p->vy = -jump_force;
    }
    
    // Ограничение скорости
    if (p->vx > 0.3f) p->vx = 0.3f;
    if (p->vx < -0.3f) p->vx = -0.3f;
    
    // Гравитация
    p->vy += 0.02f;
    
    // Обновление позиции
    p->x += p->vx;
    p->y += p->vy;
    
    // Трение
    p->vx *= 0.9f;
    
    // Коллизия с полом (упрощенно)
    if (p->y > WORLD_HEIGHT - 2) {
        p->y = WORLD_HEIGHT - 2;
        p->vy = 0;
    }
    
    // Коллизия со стенами
    if (p->x < 0) { p->x = 0; p->vx = 0; }
    if (p->x > WORLD_WIDTH - 1) { p->x = WORLD_WIDTH - 1; p->vx = 0; }
    
    pthread_mutex_unlock(&game_mutex);
}

// Игровой цикл сервера
void* game_loop(void* arg) {
    while (server_running) {
        usleep(1000000 / TICK_RATE); // ~60 FPS
        
        pthread_mutex_lock(&game_mutex);
        
        // Обновление состояния игры здесь можно добавить
        
        // Рассылка состояния всем игрокам
        PacketGameState state = {0};
        state.type = MSG_GAME_STATE;
        state.tick = 0; // Можно добавить счетчик тиков
        state.score_blue = scores[0];
        state.score_red = scores[1];
        state.active_players = player_count;
        
        for (int i = 0; i < player_count; i++) {
            state.players[i] = players[i];
        }
        
        pthread_mutex_unlock(&game_mutex);
        
        // Отправляем всем подключенным игрокам
        // В реальной реализации нужно хранить список сокетов клиентов
        // Здесь упрощенно - предполагаем что есть глобальный массив сокетов
    }
    return NULL;
}

// Запуск сервера
int start_server() {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        return -1;
    }
    
    // Разрешаем повторное использование адреса
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        return -1;
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);
    
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        return -1;
    }
    
    if (listen(server_fd, 5) < 0) {
        perror("Listen failed");
        close(server_fd);
        return -1;
    }
    
    if (set_nonblocking(server_fd) < 0) {
        perror("Set non-blocking failed");
        close(server_fd);
        return -1;
    }
    
    printf("Server started on port %d\n", SERVER_PORT);
    server_running = true;
    
    // Запускаем игровой поток
    pthread_t game_thread;
    if (pthread_create(&game_thread, NULL, game_loop, NULL) != 0) {
        perror("Failed to create game thread");
        return -1;
    }
    
    return 0;
}

// Основной цикл принятия соединений
void server_accept_loop() {
    fd_set readfds;
    struct timeval tv;
    
    while (server_running) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        
        int activity = select(server_fd + 1, &readfds, NULL, NULL, &tv);
        
        if (activity > 0 && FD_ISSET(server_fd, &readfds)) {
            struct sockaddr_in client_addr;
            socklen_t addrlen = sizeof(client_addr);
            int client_sock = accept(server_fd, (struct sockaddr*)&client_addr, &addrlen);
            
            if (client_sock >= 0) {
                set_nonblocking(client_sock);
                
                // Читаем первый пакет (подключение)
                PacketConnect pkt;
                int bytes = recv_packet(client_sock, &pkt, sizeof(pkt));
                
                if (bytes > 0 && pkt.type == MSG_CONNECT_REQ) {
                    handle_connect_req(client_sock, &pkt, &client_addr);
                    // В полной версии нужно сохранить сокет клиента в массив
                } else {
                    close(client_sock);
                }
            }
        }
    }
}

// Остановка сервера
void stop_server() {
    server_running = false;
    if (server_fd >= 0) {
        close(server_fd);
        server_fd = -1;
    }
    printf("Server stopped\n");
}

#ifdef STANDALONE_SERVER
int main(void) {
    if (start_server() < 0) {
        return 1;
    }
    
    printf("CTF Server running. Press Ctrl+C to stop.\n");
    server_accept_loop();
    
    stop_server();
    return 0;
}
#else
// Standalone server entry point
int server_main(void) {
    if (start_server() < 0) {
        return 1;
    }
    
    printf("CTF Server running. Press Ctrl+C to stop.\n");
    server_accept_loop();
    
    stop_server();
    return 0;
}
#endif
