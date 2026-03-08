#pragma once

typedef struct {
    char *text;
    double start; // in seconds
    double end;
    int is_background;
} Word;

Word* parse_ttml(const char* path, int* word_count);
