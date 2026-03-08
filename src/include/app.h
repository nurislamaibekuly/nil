#pragma once

#include <stddef.h>

typedef struct {
    int scale;
    int padding_top;
    int padding_left;
    int clear_screen;
    int main_scale;
    int bg_scale;
    int show_background;
    int bg_requires_main;
    int persist_main_word;
    int main_use_ascii;
    int main_row_percent;
    int bg_row_from_bottom;
    char main_color[32];
    char bg_color[32];
    char ascii_font[32];
    char ascii_font_file[512];
    char fetch_url_template[512];
} AppConfig;

typedef struct {
    char song[256];
    char artist[256];
} TrackInfo;

void app_default_config(AppConfig* config);
int app_load_config(AppConfig* config, const char* path);
int app_expand_path(const char* input, char* out, size_t out_len);
int app_get_now_playing(TrackInfo* track);
int app_get_now_playing_position(double* seconds, int* is_playing);
int app_find_offline_ttml(const char* db_path, const TrackInfo* track, char* out_path, size_t out_len);
int app_fetch_online_ttml(const AppConfig* config, const TrackInfo* track, char* out_path, size_t out_len);
int app_get_ttml(const AppConfig* config, const TrackInfo* track, char* out_path, size_t out_len);