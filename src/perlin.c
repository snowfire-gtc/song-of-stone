// src/perlin.c - Реализация шума Перлина
#define _GNU_SOURCE  // Для M_PI в glibc
#include "perlin.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

// Определяем M_PI если не определено
#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#define PERLIN_REPEAT 256
#define PERLIN_TABSIZE 256

static uint8_t permutation[PERLIN_TABSIZE];
static float gradient[PERLIN_TABSIZE][2];

// Псевдослучайная инициализация
static void shuffle(uint8_t* array, int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        uint8_t temp = array[i];
        array[i] = array[j];
        array[j] = temp;
    }
}

void perlin_init(uint32_t seed) {
    srand(seed);
    
    // Инициализация перестановки
    for (int i = 0; i < PERLIN_TABSIZE; i++) {
        permutation[i] = i;
    }
    shuffle(permutation, PERLIN_TABSIZE);
    
    // Генерация градиентов (единичные векторы)
    for (int i = 0; i < PERLIN_TABSIZE; i++) {
        float angle = (rand() / (float)RAND_MAX) * 2.0f * M_PI;
        gradient[i][0] = cosf(angle);
        gradient[i][1] = sinf(angle);
    }
}

// Функция сглаживания (fade)
static inline float fade(float t) {
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

// Линейная интерполяция
static inline float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

// Скалярное произведение градиента и вектора расстояния
static inline float dot(int hash, float x, float y) {
    int h = hash & 0xFF;
    return gradient[h][0] * x + gradient[h][1] * y;
}

float perlin_noise_1d(float x) {
    // Для 1D используем 2D с y=0
    return perlin_noise_2d(x, 0.0f);
}

float perlin_noise_2d(float x, float y) {
    // Целые координаты
    int X = (int)floorf(x) & (PERLIN_TABSIZE - 1);
    int Y = (int)floorf(y) & (PERLIN_TABSIZE - 1);
    
    // Дробные части
    float xf = x - floorf(x);
    float yf = y - floorf(y);
    
    // Векторы расстояний до углов
    float u = fade(xf);
    float v = fade(yf);
    
    // Хэши для четырёх углов
    int A = permutation[X] + Y;
    int B = permutation[(X + 1) & (PERLIN_TABSIZE - 1)] + Y;
    
    // Интерполяция значений градиентов
    float x1 = lerp(dot(permutation[A], xf, yf), 
                    dot(permutation[B], xf - 1.0f, yf), u);
    float x2 = lerp(dot(permutation[(A + 1) & (PERLIN_TABSIZE - 1)], xf, yf - 1.0f),
                    dot(permutation[(B + 1) & (PERLIN_TABSIZE - 1)], xf - 1.0f, yf - 1.0f), u);
    
    return lerp(x1, x2, v);
}

float perlin_fbm_2d(float x, float y, int octaves, float persistence, float lacunarity) {
    float total = 0.0f;
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float maxValue = 0.0f;
    
    for (int i = 0; i < octaves; i++) {
        total += perlin_noise_2d(x * frequency, y * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }
    
    return total / maxValue;
}

float perlin_normalized_2d(float x, float y, int octaves, float persistence, float lacunarity) {
    float noise = perlin_fbm_2d(x, y, octaves, persistence, lacunarity);
    // Преобразование из [-1, 1] в [0, 1]
    return (noise + 1.0f) * 0.5f;
}
