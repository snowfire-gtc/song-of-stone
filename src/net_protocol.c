// net_protocol.c - Реализация сетевого протокола с надежностью и безопасностью
#include "net_protocol.h"
#include <string.h>
#include <stdio.h>

// --- Контрольная сумма (простая реализация) ---
uint32_t calculate_checksum(const void* data, size_t len) {
    const uint8_t* bytes = (const uint8_t*)data;
    uint32_t checksum = 0;
    
    // Простая хеш-функция (можно заменить на CRC32)
    for (size_t i = 0; i < len; i++) {
        checksum = ((checksum << 5) + checksum) + bytes[i];
        checksum ^= (checksum >> 17);
    }
    
    return checksum;
}

bool verify_packet(const NetworkPacket* pkt) {
    if (!pkt) return false;
    
    // Вычисляем контрольную сумму данных
    uint32_t calculated = calculate_checksum(&pkt->data, pkt->header.size);
    
    return calculated == pkt->header.checksum;
}

// --- Сериализация/десериализация ---

size_t serialize_packet(const NetworkPacket* pkt, uint8_t* buffer, size_t buf_size) {
    if (!pkt || !buffer || buf_size < sizeof(PacketHeader)) {
        return 0;
    }
    
    size_t total_size = sizeof(PacketHeader) + pkt->header.size;
    if (total_size > buf_size) {
        return 0;
    }
    
    // Копируем заголовок
    memcpy(buffer, &pkt->header, sizeof(PacketHeader));
    
    // Копируем данные
    if (pkt->header.size > 0) {
        memcpy(buffer + sizeof(PacketHeader), &pkt->data, pkt->header.size);
    }
    
    return total_size;
}

bool deserialize_packet(const uint8_t* buffer, size_t len, NetworkPacket* pkt) {
    if (!buffer || !pkt || len < sizeof(PacketHeader)) {
        return false;
    }
    
    // Читаем заголовок
    memcpy(&pkt->header, buffer, sizeof(PacketHeader));
    
    // Проверяем размер
    size_t total_size = sizeof(PacketHeader) + pkt->header.size;
    if (total_size > len || total_size > BUFFER_SIZE) {
        return false;
    }
    
    // Читаем данные
    if (pkt->header.size > 0) {
        memcpy(&pkt->data, buffer + sizeof(PacketHeader), pkt->header.size);
    }
    
    // Проверяем контрольную сумму
    return verify_packet(pkt);
}

// --- Дельта-кодирование ---

void encode_delta(const SnapshotChar* current, const SnapshotChar* previous, DeltaPos* delta) {
    if (!current || !previous || !delta) return;
    
    delta->id = current->id;
    delta->dx = current->x - previous->x;
    delta->dy = current->y - previous->y;
    delta->dvx = current->vx - previous->vx;
    delta->dvy = current->vy - previous->vy;
}

void decode_delta(const DeltaPos* delta, SnapshotChar* previous, SnapshotChar* result) {
    if (!delta || !previous || !result) return;
    
    *result = *previous;  // Копируем предыдущее состояние
    result->id = delta->id;
    result->x = previous->x + delta->dx;
    result->y = previous->y + delta->dy;
    result->vx = previous->vx + delta->dvx;
    result->vy = previous->vy + delta->dvy;
}

// --- Сжатие изменений блоков (простое RLE) ---

size_t compress_block_changes(const BlockChange* changes, uint8_t count, uint8_t* output) {
    if (!changes || !output || count == 0) return 0;
    
    size_t out_idx = 0;
    uint8_t i = 0;
    
    while (i < count && out_idx < BUFFER_SIZE - 3) {
        uint8_t run_length = 1;
        
        // Считаем повторяющиеся блоки
        while (i + run_length < count && 
               run_length < 255 &&
               changes[i + run_length].block_id == changes[i].block_id &&
               changes[i + run_length].action == changes[i].action) {
            run_length++;
        }
        
        // Записываем: блок, действие, длина
        output[out_idx++] = changes[i].block_id;
        output[out_idx++] = changes[i].action;
        output[out_idx++] = run_length;
        
        i += run_length;
    }
    
    return out_idx;
}

size_t decompress_block_changes(const uint8_t* input, size_t len, BlockChange* output, size_t max_count) {
    if (!input || !output || len == 0) return 0;
    
    size_t in_idx = 0;
    size_t out_idx = 0;
    
    while (in_idx < len - 2 && out_idx < max_count) {
        uint8_t block_id = input[in_idx++];
        uint8_t action = input[in_idx++];
        uint8_t run_length = input[in_idx++];
        
        // Восстанавливаем повторяющиеся блоки
        for (uint8_t j = 0; j < run_length && out_idx < max_count; j++) {
            output[out_idx].block_id = block_id;
            output[out_idx].action = action;
            output[out_idx].x = 0;  // Координаты теряются при простом RLE
            output[out_idx].y = 0;
            output[out_idx].timestamp = 0;
            out_idx++;
        }
    }
    
    return out_idx;
}

// --- Вспомогательные функции для отладки ---

#ifdef DEBUG_NET
void print_packet_info(const NetworkPacket* pkt) {
    if (!pkt) return;
    
    const char* type_names[] = {
        "NONE", "HELLO", "WELCOME", "INPUT", "SNAPSHOT", 
        "DELTA", "BLOCK_CHANGE", "CHAT", "DISCONNECT", "ACK", "HEARTBEAT"
    };
    
    printf("[NET] Packet: type=%s, seq=%u, ack=%u, size=%u, checksum=%u\n",
           pkt->header.type < 11 ? type_names[pkt->header.type] : "UNKNOWN",
           pkt->header.sequence,
           pkt->header.ack,
           pkt->header.size,
           pkt->header.checksum);
}
#endif