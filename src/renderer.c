#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <ttml.h>
#include <renderer.h>

static RendererConfig g_config = {
    .scale = 3,
    .padding_top = 2,
    .padding_left = 8,
    .clear_screen = 1,
    .main_scale = 3,
    .bg_scale = 1,
    .show_background = 1,
    .main_use_ascii = 1,
    .main_row_percent = 50,
    .bg_row_from_bottom = 2,
    .main_color = "97",
    .bg_color = "2;90",
    .ascii_font = "block",
    .ascii_font_file = ""
};

static int utf8_char_len(unsigned char c) {
    if ((c & 0x80) == 0) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    return 1;
}

static void load_custom_font_if_needed(void);

void renderer_set_config(const RendererConfig* config) {
    if (!config) return;
    g_config = *config;
    if (g_config.scale < 1) g_config.scale = 1;
    if (g_config.scale > 8) g_config.scale = 8;
    if (g_config.main_scale < 1) g_config.main_scale = 1;
    if (g_config.main_scale > 8) g_config.main_scale = 8;
    if (g_config.bg_scale < 1) g_config.bg_scale = 1;
    if (g_config.bg_scale > 8) g_config.bg_scale = 8;
    if (g_config.padding_top < 0) g_config.padding_top = 0;
    if (g_config.padding_left < 0) g_config.padding_left = 0;
    if (g_config.main_row_percent < 0) g_config.main_row_percent = 0;
    if (g_config.main_row_percent > 100) g_config.main_row_percent = 100;
    if (g_config.bg_row_from_bottom < 1) g_config.bg_row_from_bottom = 1;
    if (g_config.main_color[0] == '\0') snprintf(g_config.main_color, sizeof(g_config.main_color), "%s", "97");
    if (g_config.bg_color[0] == '\0') snprintf(g_config.bg_color, sizeof(g_config.bg_color), "%s", "2;90");
    if (g_config.ascii_font[0] == '\0') {
        snprintf(g_config.ascii_font, sizeof(g_config.ascii_font), "%s", "block");
    }
    load_custom_font_if_needed();
}

typedef struct {
    char ch;
    const char* rows[5];
} Glyph;

typedef struct {
    char ch;
    char rows[5][64];
} CustomGlyph;

#define MAX_CUSTOM_GLYPHS 256
static CustomGlyph g_custom_glyphs[MAX_CUSTOM_GLYPHS];
static int g_custom_glyph_count = 0;

static const Glyph BLOCK_GLYPHS[] = {
    {'A', {" ### ", "#   #", "#####", "#   #", "#   #"}},
    {'B', {"#### ", "#   #", "#### ", "#   #", "#### "}},
    {'C', {" ####", "#    ", "#    ", "#    ", " ####"}},
    {'D', {"#### ", "#   #", "#   #", "#   #", "#### "}},
    {'E', {"#####", "#    ", "#### ", "#    ", "#####"}},
    {'F', {"#####", "#    ", "#### ", "#    ", "#    "}},
    {'G', {" ####", "#    ", "#  ##", "#   #", " ####"}},
    {'H', {"#   #", "#   #", "#####", "#   #", "#   #"}},
    {'I', {"#####", "  #  ", "  #  ", "  #  ", "#####"}},
    {'J', {"#####", "   # ", "   # ", "#  # ", " ##  "}},
    {'K', {"#   #", "#  # ", "###  ", "#  # ", "#   #"}},
    {'L', {"#    ", "#    ", "#    ", "#    ", "#####"}},
    {'M', {"#   #", "## ##", "# # #", "#   #", "#   #"}},
    {'N', {"#   #", "##  #", "# # #", "#  ##", "#   #"}},
    {'O', {" ### ", "#   #", "#   #", "#   #", " ### "}},
    {'P', {"#### ", "#   #", "#### ", "#    ", "#    "}},
    {'Q', {" ### ", "#   #", "#   #", "#  ##", " ####"}},
    {'R', {"#### ", "#   #", "#### ", "#  # ", "#   #"}},
    {'S', {" ####", "#    ", " ### ", "    #", "#### "}},
    {'T', {"#####", "  #  ", "  #  ", "  #  ", "  #  "}},
    {'U', {"#   #", "#   #", "#   #", "#   #", " ### "}},
    {'V', {"#   #", "#   #", "#   #", " # # ", "  #  "}},
    {'W', {"#   #", "#   #", "# # #", "## ##", "#   #"}},
    {'X', {"#   #", " # # ", "  #  ", " # # ", "#   #"}},
    {'Y', {"#   #", " # # ", "  #  ", "  #  ", "  #  "}},
    {'Z', {"#####", "   # ", "  #  ", " #   ", "#####"}},
    {'0', {" ### ", "#  ##", "# # #", "##  #", " ### "}},
    {'1', {"  #  ", " ##  ", "  #  ", "  #  ", " ### "}},
    {'2', {" ### ", "#   #", "   # ", "  #  ", "#####"}},
    {'3', {"#### ", "    #", " ### ", "    #", "#### "}},
    {'4', {"#   #", "#   #", "#####", "    #", "    #"}},
    {'5', {"#####", "#    ", "#### ", "    #", "#### "}},
    {'6', {" ### ", "#    ", "#### ", "#   #", " ### "}},
    {'7', {"#####", "   # ", "  #  ", " #   ", "#    "}},
    {'8', {" ### ", "#   #", " ### ", "#   #", " ### "}},
    {'9', {" ### ", "#   #", " ####", "    #", " ### "}},
    {' ', {"  ", "  ", "  ", "  ", "  "}},
    {'-', {"     ", "     ", "#####", "     ", "     "}},
    {',', {"     ", "     ", "     ", " ##  ", " #   "}},
    {'.', {"     ", "     ", "     ", " ##  ", " ##  "}},
    {'(', {"   # ", "  #  ", "  #  ", "  #  ", "   # "}},
    {')', {" #   ", "  #  ", "  #  ", "  #  ", " #   "}},
    {'!', {"  #  ", "  #  ", "  #  ", "     ", "  #  "}},
    {'?', {" ### ", "#   #", "  ## ", "     ", "  #  "}},
    {'\'', {"  #  ", "  #  ", "     ", "     ", "     "}},
    {'"', {" # # ", " # # ", "     ", "     ", "     "}}
};

static const Glyph MINI_GLYPHS[] = {
    {'A', {" ##  ", "#  # ", "#### ", "#  # ", "#  # "}},
    {'B', {"###  ", "#  # ", "###  ", "#  # ", "###  "}},
    {'C', {" ### ", "#    ", "#    ", "#    ", " ### "}},
    {'D', {"###  ", "#  # ", "#  # ", "#  # ", "###  "}},
    {'E', {"#### ", "#    ", "###  ", "#    ", "#### "}},
    {'F', {"#### ", "#    ", "###  ", "#    ", "#    "}},
    {'G', {" ### ", "#    ", "# ## ", "#  # ", " ### "}},
    {'H', {"#  # ", "#  # ", "#### ", "#  # ", "#  # "}},
    {'I', {"###  ", " #   ", " #   ", " #   ", "###  "}},
    {'J', {" ### ", "  #  ", "  #  ", "# #  ", " #   "}},
    {'K', {"#  # ", "# #  ", "##   ", "# #  ", "#  # "}},
    {'L', {"#    ", "#    ", "#    ", "#    ", "#### "}},
    {'M', {"#  # ", "#### ", "# ## ", "#  # ", "#  # "}},
    {'N', {"#  # ", "## # ", "# ## ", "#  # ", "#  # "}},
    {'O', {" ##  ", "#  # ", "#  # ", "#  # ", " ##  "}},
    {'P', {"###  ", "#  # ", "###  ", "#    ", "#    "}},
    {'Q', {" ##  ", "#  # ", "#  # ", "# ## ", " ### "}},
    {'R', {"###  ", "#  # ", "###  ", "# #  ", "#  # "}},
    {'S', {" ### ", "#    ", " ##  ", "   # ", "###  "}},
    {'T', {"#### ", " #   ", " #   ", " #   ", " #   "}},
    {'U', {"#  # ", "#  # ", "#  # ", "#  # ", " ##  "}},
    {'V', {"#  # ", "#  # ", "#  # ", " ##  ", " ##  "}},
    {'W', {"#  # ", "#  # ", "# ## ", "#### ", "#  # "}},
    {'X', {"#  # ", " ##  ", " ##  ", " ##  ", "#  # "}},
    {'Y', {"#  # ", " ##  ", " #   ", " #   ", " #   "}},
    {'Z', {"#### ", "  #  ", " #   ", "#    ", "#### "}},
    {'0', {" ##  ", "# ## ", "## # ", "#  # ", " ##  "}},
    {'1', {" #   ", "##   ", " #   ", " #   ", "###  "}},
    {'2', {" ##  ", "#  # ", "  #  ", " #   ", "#### "}},
    {'3', {"###  ", "   # ", " ##  ", "   # ", "###  "}},
    {'4', {"#  # ", "#  # ", "#### ", "   # ", "   # "}},
    {'5', {"#### ", "#    ", "###  ", "   # ", "###  "}},
    {'6', {" ##  ", "#    ", "###  ", "#  # ", " ##  "}},
    {'7', {"#### ", "   # ", "  #  ", " #   ", "#    "}},
    {'8', {" ##  ", "#  # ", " ##  ", "#  # ", " ##  "}},
    {'9', {" ##  ", "#  # ", " ### ", "   # ", " ##  "}},
    {' ', {"  ", "  ", "  ", "  ", "  "}},
    {'-', {"    ", "    ", "####", "    ", "    "}},
    {',', {"    ", "    ", "    ", " ## ", " #  "}},
    {'.', {"    ", "    ", "    ", " ## ", " ## "}},
    {'(', {"  # ", " #  ", " #  ", " #  ", "  # "}},
    {')', {" #  ", "  # ", "  # ", "  # ", " #  "}},
    {'!', {" #  ", " #  ", " #  ", "    ", " #  "}},
    {'?', {" ## ", "#  #", "  # ", "    ", "  # "}},
    {'\'', {" #  ", " #  ", "    ", "    ", "    "}},
    {'"', {"# # ", "# # ", "    ", "    ", "    "}}
}; // VIBECODDEDDD!

static void trim_newline(char* s) {
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r')) s[--n] = '\0';
}

static int resolve_font_path(const char* input, char* out, size_t out_len) {
    if (!input || !out || out_len == 0) return 0;
    if (input[0] == '~' && input[1] == '/') {
        const char* home = getenv("HOME");
        if (!home) return 0;
        return snprintf(out, out_len, "%s/%s", home, input + 2) < (int)out_len;
    }
    return snprintf(out, out_len, "%s", input) < (int)out_len;
}

static char parse_symbol_token(const char* token) {
    if (strcmp(token, "SPACE") == 0) return ' ';
    if (strcmp(token, "QUOTE") == 0) return '"';
    if (strcmp(token, "APOSTROPHE") == 0) return '\'';
    if (strlen(token) == 1) return token[0];
    return '\0';
}

static void load_custom_font_if_needed(void) {
    g_custom_glyph_count = 0;
    if (strcmp(g_config.ascii_font, "custom") != 0) return;
    if (g_config.ascii_font_file[0] == '\0') return;

    char resolved[1024];
    if (!resolve_font_path(g_config.ascii_font_file, resolved, sizeof(resolved))) return;

    FILE* f = fopen(resolved, "r");
    if (!f) return;

    char line[256];
    while (g_custom_glyph_count < MAX_CUSTOM_GLYPHS && fgets(line, sizeof(line), f)) {
        trim_newline(line);
        if (line[0] == '\0') continue;

        char ch = parse_symbol_token(line);
        if (ch == '\0') continue;

        CustomGlyph g = {0};
        g.ch = ch;
        int ok = 1;
        for (int r = 0; r < 5; r++) {
            if (!fgets(line, sizeof(line), f)) {
                ok = 0;
                break;
            }
            trim_newline(line);
            snprintf(g.rows[r], sizeof(g.rows[r]), "%s", line);
        }
        if (ok) {
            g_custom_glyphs[g_custom_glyph_count++] = g;
        }
    }
    fclose(f);
}

static const Glyph* find_glyph(char c) {
    static Glyph g;
    if (strcmp(g_config.ascii_font, "custom") == 0 && g_custom_glyph_count > 0) {
        for (int i = 0; i < g_custom_glyph_count; i++) {
            if (g_custom_glyphs[i].ch == c) {
                g.ch = c;
                for (int r = 0; r < 5; r++) g.rows[r] = g_custom_glyphs[i].rows[r];
                return &g;
            }
        }
    }
    const Glyph* set = (strcmp(g_config.ascii_font, "mini") == 0) ? MINI_GLYPHS : BLOCK_GLYPHS;
    size_t count = (strcmp(g_config.ascii_font, "mini") == 0) ?
        (sizeof(MINI_GLYPHS) / sizeof(MINI_GLYPHS[0])) :
        (sizeof(BLOCK_GLYPHS) / sizeof(BLOCK_GLYPHS[0]));
    for (size_t i = 0; i < count; i++) {
        if (set[i].ch == c) return &set[i];
    }
    return NULL;
}

static int is_ascii_only(const char* s) {
    for (size_t i = 0; s[i] != '\0'; i++) {
        if ((unsigned char)s[i] > 127) return 0;
    }
    return 1;
}

static void get_terminal_size(int* cols, int* rows) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0) {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return;
    }
    *cols = 80;
    *rows = 24;
}

static int count_codepoints(const char* s) {
    int n = 0;
    for (int i = 0; s[i] != '\0'; ) {
        i += utf8_char_len((unsigned char)s[i]);
        n++;
    }
    return n;
}

static int ascii_text_width(const char* text) {
    int max_width = 0;
    int height = 5;
    for (int r = 0; r < height; r++) {
        int row_width = 0;
        for (size_t i = 0; text[i] != '\0'; i++) {
            char c = (char)toupper((unsigned char)text[i]);
            const Glyph* g = find_glyph(c);
            const char* row = g ? g->rows[r] : "???";
            row_width += (int)strlen(row) * g_config.scale + 1;
        }
        if (row_width > max_width) max_width = row_width;
    }
    return max_width;
}

static void render_ascii_text(const char* text, int left_margin) {
    int height = 5;
    for (int r = 0; r < height; r++) {
        for (int vs = 0; vs < g_config.scale; vs++) {
            for (int pad = 0; pad < left_margin; pad++) putchar(' ');
            for (size_t i = 0; text[i] != '\0'; i++) {
                char c = (char)toupper((unsigned char)text[i]);
                const Glyph* g = find_glyph(c);
                const char* row = g ? g->rows[r] : "???";
                for (size_t j = 0; row[j] != '\0'; j++) {
                    for (int hs = 0; hs < g_config.scale; hs++) {
                        if (row[j] == '#') {
                            fputs("█", stdout);
                        } else {
                            putchar(row[j]);
                        }
                    }
                }
                putchar(' ');
            }
            putchar('\n');
        }
    }
}

static void render_ascii_text_at(const char* text, int top_row, int left_col, const char* color) {
    int height = 5;
    for (int r = 0; r < height; r++) {
        for (int vs = 0; vs < g_config.scale; vs++) {
            int row = top_row + r * g_config.scale + vs;
            printf("\033[%d;%dH%s", row, left_col, color);
            for (size_t i = 0; text[i] != '\0'; i++) {
                char c = (char)toupper((unsigned char)text[i]);
                const Glyph* g = find_glyph(c);
                const char* glyph_row = g ? g->rows[r] : "???";
                for (size_t j = 0; glyph_row[j] != '\0'; j++) {
                    for (int hs = 0; hs < g_config.scale; hs++) {
                        if (glyph_row[j] == '#') {
                            fputs("█", stdout);
                        } else {
                            putchar(glyph_row[j]);
                        }
                    }
                }
                putchar(' ');
            }
            fputs("\033[0m", stdout);
        }
    }
}

static void render_utf8_scaled_at(const char* text, int row, int col, const char* color, int letter_spacing) {
    for (int v = 0; v < g_config.scale; v++) {
        printf("\033[%d;%dH%s", row + v, col, color);
        int text_len = 0;
        while (text[text_len] != '\0') text_len++;
        int i = 0;
        while (i < text_len) {
            int cp_len = utf8_char_len((unsigned char)text[i]);
            for (int h = 0; h < g_config.scale; h++) {
                fwrite(&text[i], 1, (size_t)cp_len, stdout);
            }
            if (letter_spacing > 0) {
                for (int s = 0; s < letter_spacing; s++) putchar(' ');
            }
            i += cp_len;
        }
        fputs("\033[0m", stdout);
    }
}

void render_frame(const Word* main_word, const Word* bg_word) {
    int cols = 80;
    int rows = 24;
    get_terminal_size(&cols, &rows);
    if (g_config.clear_screen) {
        printf("\033[2J\033[H");
    }

    int saved_scale = g_config.scale;
    char main_color_esc[40];
    char bg_color_esc[40];
    snprintf(main_color_esc, sizeof(main_color_esc), "\033[%sm", g_config.main_color);
    snprintf(bg_color_esc, sizeof(bg_color_esc), "\033[%sm", g_config.bg_color);

    if (main_word && main_word->text) {
        g_config.scale = g_config.main_scale;
        if (g_config.main_use_ascii && is_ascii_only(main_word->text)) {
            int block_height = 5 * g_config.scale;
            int block_width = ascii_text_width(main_word->text);
            int row = (rows * g_config.main_row_percent) / 100 - block_height / 2 + 1;
            int col = (cols - block_width) / 2 + 1;
            if (row < 1) row = 1;
            if (col < 1) col = 1;
            render_ascii_text_at(main_word->text, row, col, main_color_esc);
        } else {
            int width = count_codepoints(main_word->text) * (g_config.scale + 1);
            int row = (rows * g_config.main_row_percent) / 100 - g_config.scale / 2 + 1;
            int col = (cols - width) / 2 + 1;
            if (row < 1) row = 1;
            if (col < 1) col = 1;
            render_utf8_scaled_at(main_word->text, row, col, main_color_esc, 1);
        }
    }

    if (g_config.show_background && bg_word && bg_word->text) {
        g_config.scale = g_config.bg_scale;
        int width = count_codepoints(bg_word->text) * g_config.scale;
        int row = rows - g_config.bg_row_from_bottom;
        int col = (cols - width) / 2 + 1;
        if (row < 1) row = 1;
        if (col < 1) col = 1;
        render_utf8_scaled_at(bg_word->text, row, col, bg_color_esc, 0);
    }
    g_config.scale = saved_scale;
    fflush(stdout);
}

void render_word(Word* w) {
    int cols = 80;
    int rows = 24;
    get_terminal_size(&cols, &rows);

    int is_ascii = is_ascii_only(w->text);
    int block_height = is_ascii ? (5 * g_config.scale) : g_config.scale;
    int block_width = is_ascii ? ascii_text_width(w->text) : (count_codepoints(w->text) * (g_config.scale + 1));
    int top_margin = (rows - block_height) / 2;
    int left_margin = (cols - block_width) / 2;
    if (top_margin < 0) top_margin = 0;
    if (left_margin < 0) left_margin = 0;
    top_margin += g_config.padding_top;
    left_margin += g_config.padding_left;

    if (g_config.clear_screen) {
        printf("\033[2J\033[H");
    }
    for (int i = 0; i < top_margin; i++) {
        putchar('\n');
    }

    if (is_ascii) {
        render_ascii_text(w->text, left_margin);
    } else {
        int text_len = 0;
        while (w->text[text_len] != '\0') text_len++;
        for (int v = 0; v < g_config.scale; v++) {
            for (int pad = 0; pad < left_margin; pad++) putchar(' ');
            int i = 0;
            while (i < text_len) {
                int cp_len = utf8_char_len((unsigned char)w->text[i]);
                for (int h = 0; h < g_config.scale; h++) {
                    fwrite(&w->text[i], 1, (size_t)cp_len, stdout);
                }
                putchar(' ');
                i += cp_len;
            }
            putchar('\n');
        }
    }
    fflush(stdout);
}
