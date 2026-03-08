#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <ttml.h>
#include <renderer.h>
#include <app.h>

static int find_latest_started_main(const Word* words, int count, double elapsed) {
    int idx = -1;
    for (int i = 0; i < count; i++) {
        if (!words[i].is_background && words[i].start <= elapsed) {
            idx = i;
        }
    }
    return idx;
}

static int find_active_word(const Word* words, int count, double elapsed, int is_background) {
    int idx = -1;
    for (int i = 0; i < count; i++) {
        if (words[i].is_background == is_background &&
            words[i].start <= elapsed &&
            elapsed <= words[i].end) {
            idx = i;
        }
    }
    return idx;
}

int main(int argc, char** argv) {
    AppConfig config;
    app_default_config(&config);
    app_load_config(&config, "~/.config/nil/config.json");

    RendererConfig renderer_config = {
        .scale = config.scale,
        .padding_top = config.padding_top,
        .padding_left = config.padding_left,
        .clear_screen = config.clear_screen,
        .main_scale = config.main_scale,
        .bg_scale = config.bg_scale,
        .show_background = config.show_background,
        .main_use_ascii = config.main_use_ascii,
        .main_row_percent = config.main_row_percent,
        .bg_row_from_bottom = config.bg_row_from_bottom
    };
    snprintf(renderer_config.main_color, sizeof(renderer_config.main_color), "%s", config.main_color);
    snprintf(renderer_config.bg_color, sizeof(renderer_config.bg_color), "%s", config.bg_color);
    snprintf(renderer_config.ascii_font, sizeof(renderer_config.ascii_font), "%s", config.ascii_font);
    snprintf(renderer_config.ascii_font_file, sizeof(renderer_config.ascii_font_file), "%s", config.ascii_font_file);
    renderer_set_config(&renderer_config);

    char input_path_buf[1024] = {0};
    const char* input_path = NULL;
    int use_live_timeline = 0;

    if (argc > 1) {
        if (!app_expand_path(argv[1], input_path_buf, sizeof(input_path_buf))) {
            fprintf(stderr, "oopsie invalid path bestie: %s\n", argv[1]);
            return 1;
        }
        input_path = input_path_buf;
    } else {
        TrackInfo track = {0};
        if (!app_get_now_playing(&track)) {
            fprintf(stderr, "no track vibing rn from supported players :(\n");
            fprintf(stderr, "pass a ttml path or make ~/.config/nil/ttml.json pls\n");
            return 1;
        }
        use_live_timeline = 1;

        if (app_get_ttml(&config, &track, input_path_buf, sizeof(input_path_buf))) {
            input_path = input_path_buf;
        } else {
            fprintf(stderr, "no ttml found for %s - %s\n", track.song, track.artist);
            fprintf(stderr, "add it in ~/.config/nil/ttml.json or set fetch_url_template in ~/.config/nil/config.json\n");
            return 1;
        }
    }

    int word_count;
    Word* words = parse_ttml(input_path, &word_count);
    if (!words) {
        fprintf(stderr, "couldnt parse this ttml homie: %s\n", input_path);
        return 1;
    }
    if (word_count == 0) {
        fprintf(stderr, "zero timed words found in this ttml: %s\n", input_path);
        free(words);
        return 1;
    }

    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    double last_main_end = 0.0;
    for (int i = 0; i < word_count; i++) {
        if (!words[i].is_background && words[i].end > last_main_end) last_main_end = words[i].end;
    }

    int last_main_rendered = -2;
    int last_bg_rendered = -2;
    while (1) {
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        double elapsed = now.tv_sec - start_time.tv_sec + (now.tv_nsec - start_time.tv_nsec)/1e9;

        if (use_live_timeline) {
            double player_seconds = 0.0;
            int is_playing = 1;
            if (app_get_now_playing_position(&player_seconds, &is_playing)) {
                elapsed = player_seconds;
                if (!is_playing) {
                    usleep(100000);
                    continue;
                }
            }
        }

        int main_idx = config.persist_main_word ?
            find_latest_started_main(words, word_count, elapsed) :
            find_active_word(words, word_count, elapsed, 0);
        int bg_idx = config.show_background ? find_active_word(words, word_count, elapsed, 1) : -1;
        if (config.bg_requires_main && main_idx < 0) {
            bg_idx = -1;
        }

        if (main_idx != last_main_rendered || bg_idx != last_bg_rendered) {
            const Word* main_word = (main_idx >= 0) ? &words[main_idx] : NULL;
            const Word* bg_word = (bg_idx >= 0) ? &words[bg_idx] : NULL;
            render_frame(main_word, bg_word);
            last_main_rendered = main_idx;
            last_bg_rendered = bg_idx;
        }

        if (elapsed > last_main_end + 0.5) {
            break;
        }

        usleep(10000);
    }

    for (int i = 0; i < word_count; i++) {
        free(words[i].text);
    }
    free(words);
    return 0;
}
