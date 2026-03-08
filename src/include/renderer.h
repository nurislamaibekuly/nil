#pragma once
#include <ttml.h>

typedef struct {
    int scale;
    int padding_top;
    int padding_left;
    int clear_screen;
    int main_scale;
    int bg_scale;
    int show_background;
    int main_use_ascii;
    int main_row_percent;
    int bg_row_from_bottom;
    char main_color[32];
    char bg_color[32];
    char ascii_font[32];
    char ascii_font_file[512];
} RendererConfig;

void renderer_set_config(const RendererConfig* config);
void render_frame(const Word* main_word, const Word* bg_word);
void render_word(Word* w);
