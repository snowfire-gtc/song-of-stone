// src/perlin.h - Шум Перлина для процедурной генерации
#ifndef PERLIN_H
#define PERLIN_H

#include <stdint.h>

// Инициализация генератора шума с seed
void perlin_init(uint32_t seed);

// 1D шум Перлина (возвращает значение от -1.0 до 1.0)
float perlin_noise_1d(float x);

// 2D шум Перлина (возвращает значение от -1.0 до 1.0)
float perlin_noise_2d(float x, float y);

// Фрактальный шум (октавы) для более детализированного ландшафта
float perlin_fbm_2d(float x, float y, int octaves, float persistence, float lacunarity);

// Нормализованный шум (от 0.0 до 1.0)
float perlin_normalized_2d(float x, float y, int octaves, float persistence, float lacunarity);

#endif // PERLIN_H
