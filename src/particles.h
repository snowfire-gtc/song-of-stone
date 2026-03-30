// particles.h
#ifndef PARTICLES_H
#define PARTICLES_H

#include "common_game.h"
#include "raylib.h"

#define MAX_PARTICLES 500

typedef enum {
    PARTICLE_DIRT,
    PARTICLE_STONE,
    PARTICLE_WOOD,
    PARTICLE_GOLD,
    PARTICLE_LEAF,
    PARTICLE_SMOKE,
    PARTICLE_EXPLOSION,
    PARTICLE_SPARK,
    PARTICLE_WATER,
    PARTICLE_LAVA,
    PARTICLE_FLAG
} ParticleType;

typedef struct {
    float x, y;           // позиция
    float vx, vy;         // скорость
    float life;           // текущая жизнь
    float max_life;       // максимальная жизнь
    Color color;          // цвет
    float size;           // размер
    bool active;          // активна ли частица
    ParticleType type;    // тип частицы
    float rotation;       // вращение
    float rotation_speed; // скорость вращения
} Particle;

typedef struct {
    Particle particles[MAX_PARTICLES];
    int count;
} ParticleSystem;

// Инициализация системы частиц
void particles_init(ParticleSystem* ps);

// Обновление всех частиц
void particles_update(ParticleSystem* ps, float dt);

// Отрисовка всех частиц
void particles_draw(const ParticleSystem* ps);

// Создание частиц при разрушении блока
void particles_spawn_block_break(ParticleSystem* ps, float x, float y, BlockType block_type, int amount);

// Создание частиц при движении (шаги)
void particles_spawn_step(ParticleSystem* ps, float x, float y, BlockType ground_type);

// Создание частиц взрыва
void particles_spawn_explosion(ParticleSystem* ps, float x, float y, int radius);

// Создание частиц дыма
void particles_spawn_smoke(ParticleSystem* ps, float x, float y, int amount);

// Создание искр
void particles_spawn_sparks(ParticleSystem* ps, float x, float y, int amount);

// Создание частиц флага
void particles_spawn_flag_capture(ParticleSystem* ps, float x, float y, Team team);

#endif // PARTICLES_H
