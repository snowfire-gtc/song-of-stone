// particles.c
#include "particles.h"
#include "stdlib.h"
#include "math.h"

// Глобальная система частиц (определяется здесь, используется в main.c и logic_*.c)
ParticleSystem g_particles;

static Color get_particle_color(ParticleType type) {
    switch (type) {
        case PARTICLE_DIRT: return (Color){139, 69, 19, 255};      // коричневый
        case PARTICLE_STONE: return (Color){128, 128, 128, 255};   // серый
        case PARTICLE_WOOD: return (Color){101, 67, 33, 255};      // тёмно-коричневый
        case PARTICLE_GOLD: return (Color){255, 215, 0, 255};      // золотой
        case PARTICLE_LEAF: return (Color){34, 139, 34, 255};      // зелёный
        case PARTICLE_SMOKE: return (Color){80, 80, 80, 180};      // тёмно-серый полупрозрачный
        case PARTICLE_EXPLOSION: return (Color){255, 100, 0, 255}; // оранжево-красный
        case PARTICLE_SPARK: return (Color){255, 255, 100, 255};   // жёлтый
        case PARTICLE_WATER: return (Color){30, 144, 255, 200};    // голубой
        case PARTICLE_LAVA: return (Color){255, 69, 0, 255};       // красно-оранжевый
        case PARTICLE_FLAG: return (Color){255, 255, 255, 255};    // белый
        default: return WHITE;
    }
}

void particles_init(ParticleSystem* ps) {
    ps->count = 0;
    for (int i = 0; i < MAX_PARTICLES; i++) {
        ps->particles[i].active = false;
    }
}

static Particle* spawn_particle(ParticleSystem* ps) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!ps->particles[i].active) {
            ps->particles[i].active = true;
            ps->particles[i].life = 0.0f;
            return &ps->particles[i];
        }
    }
    return NULL; // нет свободных частиц
}

void particles_update(ParticleSystem* ps, float dt) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle* p = &ps->particles[i];
        if (!p->active) continue;
        
        p->life += dt;
        if (p->life >= p->max_life) {
            p->active = false;
            continue;
        }
        
        // Обновление позиции
        p->x += p->vx * dt;
        p->y += p->vy * dt;
        
        // Гравитация для некоторых типов
        if (p->type != PARTICLE_SMOKE && p->type != PARTICLE_SPARK) {
            p->vy += 200.0f * dt; // гравитация
        }
        
        // Затухание размера
        float life_ratio = 1.0f - (p->life / p->max_life);
        p->size = p->size * 0.98f + 1.0f * life_ratio;
        
        // Вращение
        p->rotation += p->rotation_speed * dt;
        
        // Трение
        p->vx *= 0.95f;
        p->vy *= 0.95f;
    }
}

void particles_draw(const ParticleSystem* ps) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        const Particle* p = &ps->particles[i];
        if (!p->active) continue;
        
        float alpha_ratio = 1.0f - (p->life / p->max_life);
        Color col = p->color;
        col.a = (unsigned char)(col.a * alpha_ratio);
        
        // Рисуем как квадрат с вращением
        Vector2 center = {p->x, p->y};
        DrawRectanglePro(
            (Rectangle){p->x - p->size/2, p->y - p->size/2, p->size, p->size},
            (Vector2){p->size/2, p->size/2},
            p->rotation * RAD2DEG,
            col
        );
    }
}

void particles_spawn_block_break(ParticleSystem* ps, float x, float y, BlockType block_type, int amount) {
    ParticleType ptype;
    switch (block_type) {
        case BLOCK_DIRT: ptype = PARTICLE_DIRT; break;
        case BLOCK_STONE: ptype = PARTICLE_STONE; break;
        case BLOCK_WOOD: ptype = PARTICLE_WOOD; break;
        case BLOCK_GOLD: ptype = PARTICLE_GOLD; break;
        case BLOCK_LEAFS: ptype = PARTICLE_LEAF; break;
        default: ptype = PARTICLE_DIRT;
    }
    
    Color col = get_particle_color(ptype);
    
    for (int i = 0; i < amount; i++) {
        Particle* p = spawn_particle(ps);
        if (!p) break;
        
        p->type = ptype;
        p->x = x + (rand() % 16) - 8;
        p->y = y + (rand() % 16) - 8;
        p->vx = ((float)(rand() % 100) - 50) / 10.0f;
        p->vy = ((float)(rand() % 100) - 50) / 10.0f - 50.0f;
        p->life = 0.0f;
        p->max_life = 0.3f + (rand() % 10) / 30.0f;
        p->color = col;
        p->size = 4.0f + (rand() % 4);
        p->rotation = (float)(rand() % 360);
        p->rotation_speed = ((float)(rand() % 200) - 100) / 10.0f;
    }
}

void particles_spawn_step(ParticleSystem* ps, float x, float y, BlockType ground_type) {
    // Небольшое количество пыли при шаге
    ParticleType ptype = (ground_type == BLOCK_STONE) ? PARTICLE_STONE : PARTICLE_DIRT;
    Color col = get_particle_color(ptype);
    
    for (int i = 0; i < 2; i++) {
        Particle* p = spawn_particle(ps);
        if (!p) break;
        
        p->type = ptype;
        p->x = x + (rand() % 8) - 4;
        p->y = y;
        p->vx = ((float)(rand() % 20) - 10) / 20.0f;
        p->vy = -((float)(rand() % 20) + 10) / 10.0f;
        p->life = 0.0f;
        p->max_life = 0.2f;
        p->color = col;
        p->size = 2.0f + (rand() % 2);
        p->rotation = 0;
        p->rotation_speed = 0;
    }
}

void particles_spawn_explosion(ParticleSystem* ps, float x, float y, int radius) {
    int count = radius * 20;
    
    // Огненные частицы
    for (int i = 0; i < count; i++) {
        Particle* p = spawn_particle(ps);
        if (!p) break;
        
        p->type = PARTICLE_EXPLOSION;
        float angle = (float)(rand() % 360) * DEG2RAD;
        float speed = (float)(rand() % 200) / 20.0f + 50.0f;
        p->x = x;
        p->y = y;
        p->vx = cosf(angle) * speed;
        p->vy = sinf(angle) * speed;
        p->life = 0.0f;
        p->max_life = 0.5f + (rand() % 10) / 20.0f;
        p->color = get_particle_color(PARTICLE_EXPLOSION);
        p->size = 6.0f + (rand() % 6);
        p->rotation = 0;
        p->rotation_speed = 0;
    }
    
    // Дым
    particles_spawn_smoke(ps, x, y, count / 2);
    
    // Искры
    particles_spawn_sparks(ps, x, y, count / 3);
}

void particles_spawn_smoke(ParticleSystem* ps, float x, float y, int amount) {
    for (int i = 0; i < amount; i++) {
        Particle* p = spawn_particle(ps);
        if (!p) break;
        
        p->type = PARTICLE_SMOKE;
        p->x = x + (rand() % 32) - 16;
        p->y = y + (rand() % 32) - 16;
        p->vx = ((float)(rand() % 40) - 20) / 20.0f;
        p->vy = -((float)(rand() % 30) + 20) / 20.0f;
        p->life = 0.0f;
        p->max_life = 1.0f + (rand() % 10) / 10.0f;
        p->color = get_particle_color(PARTICLE_SMOKE);
        p->size = 8.0f + (rand() % 8);
        p->rotation = 0;
        p->rotation_speed = ((float)(rand() % 100) - 50) / 50.0f;
    }
}

void particles_spawn_sparks(ParticleSystem* ps, float x, float y, int amount) {
    for (int i = 0; i < amount; i++) {
        Particle* p = spawn_particle(ps);
        if (!p) break;
        
        p->type = PARTICLE_SPARK;
        float angle = (float)(rand() % 360) * DEG2RAD;
        float speed = (float)(rand() % 300) / 20.0f + 100.0f;
        p->x = x;
        p->y = y;
        p->vx = cosf(angle) * speed;
        p->vy = sinf(angle) * speed;
        p->life = 0.0f;
        p->max_life = 0.3f + (rand() % 10) / 30.0f;
        p->color = get_particle_color(PARTICLE_SPARK);
        p->size = 3.0f + (rand() % 3);
        p->rotation = 0;
        p->rotation_speed = 0;
    }
}

void particles_spawn_flag_capture(ParticleSystem* ps, float x, float y, Team team) {
    Color col = (team == TEAM_BLUE) ? BLUE : RED;
    
    for (int i = 0; i < 30; i++) {
        Particle* p = spawn_particle(ps);
        if (!p) break;
        
        p->type = PARTICLE_FLAG;
        p->x = x + (rand() % 48) - 24;
        p->y = y + (rand() % 48) - 24;
        p->vx = ((float)(rand() % 100) - 50) / 20.0f;
        p->vy = -((float)(rand() % 100) + 50) / 20.0f;
        p->life = 0.0f;
        p->max_life = 0.8f + (rand() % 10) / 20.0f;
        p->color = col;
        p->size = 5.0f + (rand() % 5);
        p->rotation = (float)(rand() % 360);
        p->rotation_speed = ((float)(rand() % 200) - 100) / 10.0f;
    }
}
