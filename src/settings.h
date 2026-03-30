#ifndef SETTINGS_H
#define SETTINGS_H

#include "raylib.h"
#include <stdbool.h>

// Настройки видео
typedef struct {
    int screen_width;
    int screen_height;
    bool fullscreen;
    int vsync;
    int fps_limit;
    float render_scale;
    bool show_fps;
    bool particles_enabled;
    bool shadows_enabled;
} VideoSettings;

// Настройки звука
typedef struct {
    float master_volume;
    float music_volume;
    float sfx_volume;
    bool muted;
} AudioSettings;

// Настройки управления
typedef struct {
    // Клавиши движения
    KeyboardKey move_up;
    KeyboardKey move_down;
    KeyboardKey move_left;
    KeyboardKey move_right;
    KeyboardKey jump;
    KeyboardKey attack;
    KeyboardKey build;
    KeyboardKey use_item;
    KeyboardKey inventory;
    KeyboardKey pause;
    
    // Мышь
    float mouse_sensitivity;
    bool invert_mouse_y;
} ControlSettings;

// Внешний облик персонажа
typedef struct {
    int skin_color;      // Индекс цвета кожи
    int hair_color;      // Индекс цвета волос
    int shirt_color;     // Индекс цвета рубашки
    int pants_color;     // Индекс цвета брюк
    int hat_type;        // Тип головного убора
    int accessory_type;  // Тип аксессуара
    char name[32];       // Имя персонажа
} AppearanceSettings;

// Общие настройки
typedef struct {
    VideoSettings video;
    AudioSettings audio;
    ControlSettings controls;
    AppearanceSettings appearance;
    
    // Сетевые настройки
    char last_server_ip[64];
    int last_server_port;
    
    // Прочее
    bool auto_save;
    int autosave_interval_minutes;
} Settings;

// Инициализация настроек по умолчанию
void settings_init(Settings* settings);

// Загрузка настроек из файла
bool settings_load(Settings* settings, const char* filename);

// Сохранение настроек в файл
bool settings_save(const Settings* settings, const char* filename);

// Применение настроек видео
void settings_apply_video(const VideoSettings* video);

// Применение настроек звука
void settings_apply_audio(const AudioSettings* audio);

#endif // SETTINGS_H
