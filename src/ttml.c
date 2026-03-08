#include <ttml.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    double begin;
    double end;
    int has_begin;
    int has_end;
    int is_bg;
    int has_element_child;
    int text_len;
    int text_cap;
    char* text;
} SpanCtx;

static double parse_time_seconds(const char* value) {
    int h = 0;
    int m = 0;
    double s = 0.0;
    int parsed = sscanf(value, "%d:%d:%lf", &h, &m, &s);
    if (parsed == 3) return (double)h * 3600.0 + (double)m * 60.0 + s;
    parsed = sscanf(value, "%d:%lf", &m, &s);
    if (parsed == 2) return (double)m * 60.0 + s;
    return atof(value);
}

static int read_file(const char* path, char** out_text, size_t* out_size) {
    FILE* f = fopen(path, "rb");
    if (!f) return 0;
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return 0;
    }
    long sz = ftell(f);
    if (sz < 0) {
        fclose(f);
        return 0;
    }
    if (fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return 0;
    }
    char* buf = malloc((size_t)sz + 1);
    if (!buf) {
        fclose(f);
        return 0;
    }
    size_t n = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    buf[n] = '\0';
    *out_text = buf;
    if (out_size) *out_size = n;
    return 1;
}

static int is_name_char(char c) {
    return isalnum((unsigned char)c) || c == '_' || c == '-' || c == ':' || c == '.';
}

static void skip_ws(const char** p, const char* end) {
    while (*p < end && isspace((unsigned char)**p)) (*p)++;
}

static int starts_with_case_insensitive(const char* s, const char* lit, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (lit[i] == '\0') return 1;
        if (tolower((unsigned char)s[i]) != tolower((unsigned char)lit[i])) return 0;
    }
    return lit[n] == '\0';
}

static void append_text_char(SpanCtx* ctx, char ch) {
    if (!ctx) return;
    if (ctx->text_len + 2 > ctx->text_cap) {
        int next_cap = ctx->text_cap > 0 ? ctx->text_cap * 2 : 32;
        char* next = realloc(ctx->text, (size_t)next_cap);
        if (!next) return;
        ctx->text = next;
        ctx->text_cap = next_cap;
    }
    ctx->text[ctx->text_len++] = ch;
    ctx->text[ctx->text_len] = '\0';
}

static void append_decoded_text(SpanCtx* ctx, const char* s, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (s[i] == '&') {
            if (i + 4 <= n && strncmp(&s[i], "&lt;", 4) == 0) {
                append_text_char(ctx, '<');
                i += 3;
                continue;
            }
            if (i + 4 <= n && strncmp(&s[i], "&gt;", 4) == 0) {
                append_text_char(ctx, '>');
                i += 3;
                continue;
            }
            if (i + 5 <= n && strncmp(&s[i], "&amp;", 5) == 0) {
                append_text_char(ctx, '&');
                i += 4;
                continue;
            }
            if (i + 6 <= n && strncmp(&s[i], "&quot;", 6) == 0) {
                append_text_char(ctx, '"');
                i += 5;
                continue;
            }
            if (i + 6 <= n && strncmp(&s[i], "&apos;", 6) == 0) {
                append_text_char(ctx, '\'');
                i += 5;
                continue;
            }
        }
        append_text_char(ctx, s[i]);
    }
}

static void trim_span_text(SpanCtx* ctx) {
    while (ctx->text_len > 0 && isspace((unsigned char)ctx->text[ctx->text_len - 1])) {
        ctx->text[--ctx->text_len] = '\0';
    }
    int start = 0;
    while (start < ctx->text_len && isspace((unsigned char)ctx->text[start])) start++;
    if (start > 0 && start < ctx->text_len) {
        memmove(ctx->text, ctx->text + start, (size_t)(ctx->text_len - start + 1));
        ctx->text_len -= start;
    } else if (start >= ctx->text_len) {
        ctx->text_len = 0;
        if (ctx->text) ctx->text[0] = '\0';
    }
}

static int ensure_words_cap(Word** words, int* cap, int need_count) {
    if (need_count <= *cap) return 1;
    int next_cap = (*cap > 0) ? (*cap * 2) : 64;
    while (next_cap < need_count) next_cap *= 2;
    Word* next = realloc(*words, (size_t)next_cap * sizeof(Word));
    if (!next) return 0;
    *words = next;
    *cap = next_cap;
    return 1;
}

static void free_span_stack(SpanCtx* stack, int depth) {
    for (int i = 0; i < depth; i++) free(stack[i].text);
}

Word* parse_ttml(const char* path, int* word_count) {
    if (!word_count) return NULL;
    *word_count = 0;

    char* text = NULL;
    size_t size = 0;
    if (!read_file(path, &text, &size)) return NULL;

    Word* words = NULL;
    int words_count = 0;
    int words_cap = 0;

    SpanCtx* stack = NULL;
    int depth = 0;
    int stack_cap = 0;

    const char* p = text;
    const char* end = text + size;

    while (p < end) {
        if (*p != '<') {
            if (depth > 0) {
                const char* chunk_start = p;
                while (p < end && *p != '<') p++;
                append_decoded_text(&stack[depth - 1], chunk_start, (size_t)(p - chunk_start));
            } else {
                p++;
            }
            continue;
        }

        if (p + 4 <= end && strncmp(p, "<!--", 4) == 0) {
            const char* close = strstr(p + 4, "-->");
            if (!close) break;
            p = close + 3;
            continue;
        }
        if (p + 2 <= end && p[1] == '?') {
            const char* close = strstr(p + 2, "?>");
            if (!close) break;
            p = close + 2;
            continue;
        }
        if (p + 2 <= end && p[1] == '!') {
            const char* close = strchr(p + 2, '>');
            if (!close) break;
            p = close + 1;
            continue;
        }

        const char* tag_start = p + 1;
        int is_close = 0;
        if (tag_start < end && *tag_start == '/') {
            is_close = 1;
            tag_start++;
        }

        const char* name_start = tag_start;
        while (tag_start < end && is_name_char(*tag_start)) tag_start++;
        if (tag_start == name_start) {
            p++;
            continue;
        }

        size_t tag_name_len = (size_t)(tag_start - name_start);
        int is_span = (tag_name_len == 4 && starts_with_case_insensitive(name_start, "span", 4));

        if (is_close) {
            const char* close = strchr(tag_start, '>');
            if (!close) break;
            if (is_span && depth > 0) {
                SpanCtx ctx = stack[depth - 1];
                depth--;
                trim_span_text(&ctx);
                if (ctx.has_begin && ctx.has_end && !ctx.has_element_child && ctx.text_len > 0) {
                    if (ensure_words_cap(&words, &words_cap, words_count + 1)) {
                        words[words_count].text = ctx.text;
                        words[words_count].start = ctx.begin;
                        words[words_count].end = ctx.end;
                        words[words_count].is_background = ctx.is_bg;
                        words_count++;
                        ctx.text = NULL;
                    }
                }
                free(ctx.text);
            }
            p = close + 1;
            continue;
        }

        const char* attrs = tag_start;
        const char* close = strchr(attrs, '>');
        if (!close) break;
        int self_closing = (close > attrs && *(close - 1) == '/');

        if (depth > 0) stack[depth - 1].has_element_child = 1;

        if (is_span) {
            SpanCtx ctx;
            memset(&ctx, 0, sizeof(ctx));
            if (depth > 0 && stack[depth - 1].is_bg) ctx.is_bg = 1;

            const char* a = attrs;
            while (a < close) {
                skip_ws(&a, close);
                if (a >= close || *a == '/' || *a == '>') break;
                const char* key_start = a;
                while (a < close && is_name_char(*a)) a++;
                size_t key_len = (size_t)(a - key_start);
                skip_ws(&a, close);
                if (a >= close || *a != '=') {
                    while (a < close && *a != ' ' && *a != '\t' && *a != '\r' && *a != '\n' && *a != '>') a++;
                    continue;
                }
                a++;
                skip_ws(&a, close);
                if (a >= close || (*a != '"' && *a != '\'')) continue;
                char quote = *a++;
                const char* val_start = a;
                while (a < close && *a != quote) a++;
                size_t val_len = (size_t)(a - val_start);
                if (a < close) a++;

                if (key_len == 5 && strncmp(key_start, "begin", 5) == 0) {
                    char tmp[64];
                    size_t n = val_len < sizeof(tmp) - 1 ? val_len : sizeof(tmp) - 1;
                    memcpy(tmp, val_start, n);
                    tmp[n] = '\0';
                    ctx.begin = parse_time_seconds(tmp);
                    ctx.has_begin = 1;
                } else if (key_len == 3 && strncmp(key_start, "end", 3) == 0) {
                    char tmp[64];
                    size_t n = val_len < sizeof(tmp) - 1 ? val_len : sizeof(tmp) - 1;
                    memcpy(tmp, val_start, n);
                    tmp[n] = '\0';
                    ctx.end = parse_time_seconds(tmp);
                    ctx.has_end = 1;
                } else {
                    int key_is_role = (key_len == 4 && strncmp(key_start, "role", 4) == 0);
                    int key_ends_role = (key_len > 5 && strncmp(key_start + key_len - 5, ":role", 5) == 0);
                    if ((key_is_role || key_ends_role) && val_len == 4 && strncmp(val_start, "x-bg", 4) == 0) {
                        ctx.is_bg = 1;
                    }
                }
            }

            if (depth >= stack_cap) {
                int next = stack_cap > 0 ? stack_cap * 2 : 16;
                SpanCtx* next_stack = realloc(stack, (size_t)next * sizeof(SpanCtx));
                if (!next_stack) {
                    free(ctx.text);
                    free_span_stack(stack, depth);
                    free(stack);
                    free(text);
                    for (int i = 0; i < words_count; i++) free(words[i].text);
                    free(words);
                    return NULL;
                }
                stack = next_stack;
                stack_cap = next;
            }
            stack[depth++] = ctx;

            if (self_closing && depth > 0) {
                SpanCtx out = stack[depth - 1];
                depth--;
                free(out.text);
            }
        }

        p = close + 1;
    }

    free_span_stack(stack, depth);
    free(stack);
    free(text);

    *word_count = words_count;
    return words;
}
