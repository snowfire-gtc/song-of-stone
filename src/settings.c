#include "settings.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void settings_init(Settings* settings) {
    // Видео настройки по умолчанию
    settings->video.screen_width = 1280;
    settings->video.screen_height = 720;
    settings->video.fullscreen = false;
    settings->video.vsync = 1;
    settings->video.fps_limit = 60;
    settings->video.render_scale = 1.0f;
    settings->video.show_fps = false;
    settings->video.particles_enabled = true;
    settings->video.shadows_enabled = true;
    
    // Аудио настройки по умолчанию
    settings->audio.master_volume = 1.0f;
    settings->audio.music_volume = 0.8f;
    settings->audio.sfx_volume = 1.0f;
    settings->audio.muted = false;
    
    // Управление по умолчанию
    settings->controls.move_up = KEY_W;
    settings->controls.move_down = KEY_S;
    settings->controls.move_left = KEY_A;
    settings->controls.move_right = KEY_D;
    settings->controls.jump = KEY_SPACE;
    settings->controls.attack = MOUSE_LEFT_BUTTON;
    settings->controls.build = MOUSE_RIGHT_BUTTON;
    settings->controls.use_item = KEY_E;
    settings->controls.inventory = KEY_TAB;
    settings->controls.pause = KEY_ESCAPE;
    settings->controls.mouse_sensitivity = 1.0f;
    settings->controls.invert_mouse_y = false;
    
    // Внешность по умолчанию
    settings->appearance.skin_color = 0;
    settings->appearance.hair_color = 1;
    settings->appearance.shirt_color = 2;
    settings->appearance.pants_color = 3;
    settings->appearance.hat_type = 0;
    settings->appearance.accessory_type = 0;
    strcpy(settings->appearance.name, "Player");
    
    // Сетевые настройки
    strcpy(settings->last_server_ip, "127.0.0.1");
    settings->last_server_port = 5555;
    
    // Прочее
    settings->auto_save = true;
    settings->autosave_interval_minutes = 5;
}

bool settings_load(Settings* settings, const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Settings file not found, using defaults\n");
        settings_init(settings);
        return false;
    }
    
    // Упрощённый парсинг (в полной версии использовать JSON или INI)
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        char key[64], value[64];
        if (sscanf(line, "%63[^=]=%63s", key, value) == 2) {
            // Парсинг настроек (упрощённо)
            if (strcmp(key, "screen_width") == 0) 
                settings->video.screen_width = atoi(value);
            else if (strcmp(key, "screen_height") == 0)
                settings->video.screen_height = atoi(value);
            else if (strcmp(key, "fullscreen") == 0)
                settings->video.fullscreen = (strcmp(value, "true") == 0);
            else if (strcmp(key, "master_volume") == 0)
                settings->audio.master_volume = atof(value);
            else if (strcmp(key, "mouse_sensitivity") == 0)
                settings->controls.mouse_sensitivity = atof(value);
            // ... добавить остальные настройки
        }
    }
    
    fclose(file);
    printf("Settings loaded from %s\n", filename);
    return true;
}

bool settings_save(const Settings* settings, const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        printf("Failed to save settings to %s\n", filename);
        return false;
    }
    
    // Сохранение в простом формате key=value
    fprintf(file, "# Song of Stone Settings\n");
    fprintf(file, "screen_width=%d\n", settings->video.screen_width);
    fprintf(file, "screen_height=%d\n", settings->video.screen_height);
    fprintf(file, "fullscreen=%s\n", settings->video.fullscreen ? "true" : "false");
    fprintf(file, "vsync=%d\n", settings->video.vsync);
    fprintf(file, "fps_limit=%d\n", settings->video.fps_limit);
    fprintf(file, "render_scale=%.2f\n", settings->video.render_scale);
    fprintf(file, "show_fps=%s\n", settings->video.show_fps ? "true" : "false");
    fprintf(file, "particles_enabled=%s\n", settings->video.particles_enabled ? "true" : "false");
    fprintf(file, "shadows_enabled=%s\n", settings->video.shadows_enabled ? "true" : "false");
    
    fprintf(file, "master_volume=%.2f\n", settings->audio.master_volume);
    fprintf(file, "music_volume=%.2f\n", settings->audio.music_volume);
    fprintf(file, "sfx_volume=%.2f\n", settings->audio.sfx_volume);
    fprintf(file, "muted=%s\n", settings->audio.muted ? "true" : "false");
    
    fprintf(file, "mouse_sensitivity=%.2f\n", settings->controls.mouse_sensitivity);
    fprintf(file, "invert_mouse_y=%s\n", settings->controls.invert_mouse_y ? "true" : "false");
    
    fprintf(file, "skin_color=%d\n", settings->appearance.skin_color);
    fprintf(file, "hair_color=%d\n", settings->appearance.hair_color);
    fprintf(file, "shirt_color=%d\n", settings->appearance.shirt_color);
    fprintf(file, "pants_color=%d\n", settings->appearance.pants_color);
    
    fprintf(file, "last_server_ip=%s\n", settings->last_server_ip);
    fprintf(file, "last_server_port=%d\n", settings->last_server_port);
    
    fprintf(file, "auto_save=%s\n", settings->auto_save ? "true" : "false");
    fprintf(file, "autosave_interval_minutes=%d\n", settings->autosave_interval_minutes);
    
    fclose(file);
    printf("Settings saved to %s\n", filename);
    return true;
}

void settings_apply_video(const VideoSettings* video) {
    // Применение настроек видео
    if (video->fullscreen != IsWindowFullscreen()) {
        ToggleFullscreen();
    }
    
    // Установка лимита FPS
    SetTargetFPS(video->fps_limit);
    
    // VSync (управляется через SetupFlags при инициализации)
    // if (video->vsync) ...
    
    printf("Video settings applied: %dx%d, fullscreen=%s, fps_limit=%d\n",
           video->screen_width, video->screen_height,
           video->fullscreen ? "true" : "false",
           video->fps_limit);
}

void settings_apply_audio(const AudioSettings* audio) {
    // Применение настроек звука
    float master_vol = audio->muted ? 0.0f : audio->master_volume;
    
    // В raylib нет глобальной громкости, нужно применять к каждому звуку
    // SetMusicVolume(music, master_vol * audio->music_volume);
    // SetSoundVolume(sound, master_vol * audio->sfx_volume);
    
    printf("Audio settings applied: master=%.2f, music=%.2f, sfx=%.2f, muted=%s\n",
           audio->master_volume, audio->music_volume, audio->sfx_volume,
           audio->muted ? "true" : "false");
}
