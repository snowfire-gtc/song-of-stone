// net_protocol.c - Реализация сетевого протокола с надежностью и безопасностью
#include "net_protocol.h"
#include <string.h>
#include <stdio.h>
#include <stddef.h>

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

bool verify_packet_header(const PacketHeader* header) {
    if (!header) return false;
    // Упрощённая проверка - только базовая валидация
    return header->type > PKT_NONE && header->type <= PKT_HEARTBEAT;
}

// --- Инициализация заголовка пакета ---
void init_packet_header(PacketHeader* header, PacketType type, uint32_t ack, uint32_t sequence) {
    if (!header) return;
    memset(header, 0, sizeof(PacketHeader));
    header->type = type;
    header->flags = 0;
    header->size = 0;
    header->sequence = sequence;
    header->ack = ack;
    header->timestamp = 0;
    header->checksum = 0;
}

// --- Сериализация/десериализация ---

size_t serialize_packet(const PacketHeader* header, const uint8_t* payload, size_t payload_size, uint8_t* buffer, size_t buf_size) {
    if (!header || !buffer || buf_size < sizeof(PacketHeader)) {
        return 0;
    }
    
    size_t total_size = sizeof(PacketHeader) + payload_size;
    if (total_size > buf_size) {
        return 0;
    }
    
    // Копируем заголовок (без контрольной суммы сначала)
    PacketHeader temp_header = *header;
    temp_header.size = (uint16_t)payload_size;
    
    memcpy(buffer, &temp_header, sizeof(PacketHeader));
    
    // Копируем данные
    if (payload_size > 0 && payload) {
        memcpy(buffer + sizeof(PacketHeader), payload, payload_size);
    }
    
    // Вычисляем контрольную сумму данных
    uint32_t checksum = calculate_checksum(payload, payload_size);
    temp_header.checksum = checksum;
    
    // Записываем контрольную сумму в заголовок
    memcpy(buffer + offsetof(PacketHeader, checksum), &checksum, sizeof(checksum));
    
    return total_size;
}

bool deserialize_packet(const uint8_t* buffer, size_t len, PacketHeader* header, uint8_t* payload, size_t payload_buf_size, size_t* payload_size) {
    if (!buffer || !header || len < sizeof(PacketHeader)) {
        return false;
    }
    
    // Читаем заголовок
    memcpy(header, buffer, sizeof(PacketHeader));
    
    // Проверяем размер
    if (header->size > payload_buf_size || header->size > BUFFER_SIZE) {
        return false;
    }
    
    // Читаем данные
    if (header->size > 0 && payload) {
        memcpy(payload, buffer + sizeof(PacketHeader), header->size);
    }
    
    if (payload_size) {
        *payload_size = header->size;
    }
    
    // Проверяем контрольную сумму
    uint32_t calculated = calculate_checksum(payload, header->size);
    return calculated == header->checksum;
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

// --- Сериализация/десериализация ввода и действий ---

size_t serialize_input(const PacketInput* input, uint8_t* buffer, size_t buf_size) {
    if (!input || !buffer || buf_size < 5) return 0;
    
    buffer[0] = input->left;
    buffer[1] = input->right;
    buffer[2] = input->jump;
    buffer[3] = input->action;
    buffer[4] = input->secondary;
    
    return 5;
}

bool deserialize_input(const uint8_t* buffer, size_t len, PacketInput* input) {
    if (!buffer || !input || len < 5) return false;
    
    input->left = buffer[0];
    input->right = buffer[1];
    input->jump = buffer[2];
    input->action = buffer[3];
    input->secondary = buffer[4];
    
    return true;
}

size_t serialize_action(const PacketAction* action, uint8_t* buffer, size_t buf_size) {
    if (!action || !buffer || buf_size < 5) return 0;
    
    buffer[0] = action->action_type;
    memcpy(buffer + 1, &action->x, sizeof(action->x));
    memcpy(buffer + 3, &action->y, sizeof(action->y));
    
    return 5;
}

bool deserialize_action(const uint8_t* buffer, size_t len, PacketAction* action) {
    if (!buffer || !action || len < 5) return false;
    
    action->action_type = buffer[0];
    memcpy(&action->x, buffer + 1, sizeof(action->x));
    memcpy(&action->y, buffer + 3, sizeof(action->y));
    
    return true;
}

// --- Кодирование персонажа для снапшота ---

void encode_character_delta(const Character* current, const Character* previous, SnapshotChar* out) {
    if (!current || !out) return;
    
    out->id = (uint16_t)(current - previous); // Индекс персонажа
    out->x = (int16_t)current->x;
    out->y = (int16_t)current->y;
    out->vx = (int8_t)(current->vx * 4);
    out->vy = (int8_t)(current->vy * 4);
    out->hp = (uint8_t)current->hp;
    out->anim_state = (uint8_t)current->anim_state;
    out->team = (uint8_t)current->team;
    out->type = (uint8_t)current->type;
    out->is_shield_active = current->is_shield_active ? 1 : 0;
    out->is_aiming = current->is_aiming ? 1 : 0;
    out->is_holding_flag = current->is_holding_flag ? 1 : 0;
    out->is_climbing = current->is_climbing ? 1 : 0;
    out->is_alive = (current->hp > 0) ? 1 : 0;
    out->padding = 0;
    out->coins = current->coins;
    out->wood = current->wood;
    out->stone = current->stone;
    out->arrows = current->arrows;
    out->bombs = current->bombs;
}

// Декодирование персонажа из снапшота
void decode_character_delta(const SnapshotChar* ec, Character* ch) {
    if (!ec || !ch) return;
    
    ch->x = ec->x;
    ch->y = ec->y;
    ch->vx = ec->vx / 4.0f;
    ch->vy = ec->vy / 4.0f;
    ch->hp = ec->hp;
    ch->anim_state = (AnimationState)ec->anim_state;
    ch->team = (Team)ec->team;
    ch->type = (CharacterType)ec->type;
    ch->is_shield_active = ec->is_shield_active ? true : false;
    ch->is_aiming = ec->is_aiming ? true : false;
    ch->is_holding_flag = ec->is_holding_flag ? true : false;
    ch->is_climbing = ec->is_climbing ? true : false;
    ch->coins = ec->coins;
    ch->wood = ec->wood;
    ch->stone = ec->stone;
    ch->arrows = ec->arrows;
    ch->bombs = ec->bombs;
}

// Сериализация снапшота
size_t serialize_snapshot(const PacketSnapshot* snapshot, uint8_t* buffer, size_t buf_size) {
    if (!snapshot || !buffer || buf_size < sizeof(PacketSnapshot)) return 0;
    
    size_t offset = 0;
    
    // Копируем timestamp и weather
    memcpy(buffer + offset, &snapshot->timestamp, sizeof(snapshot->timestamp));
    offset += sizeof(snapshot->timestamp);
    
    memcpy(buffer + offset, &snapshot->weather, sizeof(snapshot->weather));
    offset += sizeof(snapshot->weather);
    
    // Количество персонажей
    memcpy(buffer + offset, &snapshot->character_count, sizeof(snapshot->character_count));
    offset += sizeof(snapshot->character_count);
    
    // Копируем данные персонажей
    size_t chars_size = snapshot->character_count * sizeof(SnapshotChar);
    if (offset + chars_size > buf_size) return 0;
    
    memcpy(buffer + offset, snapshot->characters, chars_size);
    offset += chars_size;
    
    return offset;
}

// Десериализация снапшота
bool deserialize_snapshot(const uint8_t* buffer, size_t len, PacketSnapshot* snapshot) {
    if (!buffer || !snapshot || len < sizeof(uint32_t) + sizeof(Weather) + sizeof(int)) return false;
    
    size_t offset = 0;
    
    // Читаем timestamp и weather
    memcpy(&snapshot->timestamp, buffer + offset, sizeof(snapshot->timestamp));
    offset += sizeof(snapshot->timestamp);
    
    memcpy(&snapshot->weather, buffer + offset, sizeof(snapshot->weather));
    offset += sizeof(snapshot->weather);
    
    // Количество персонажей
    memcpy(&snapshot->character_count, buffer + offset, sizeof(snapshot->character_count));
    offset += sizeof(snapshot->character_count);
    
    if (snapshot->character_count < 0 || snapshot->character_count > MAX_CHARACTERS) return false;
    
    // Читаем данные персонажей
    size_t chars_size = snapshot->character_count * sizeof(SnapshotChar);
    if (offset + chars_size > len) return false;
    
    memcpy(snapshot->characters, buffer + offset, chars_size);
    
    return true;
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