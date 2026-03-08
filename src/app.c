#include <app.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#if !defined(_WIN32)
#include <strings.h>
#endif

static int read_file(const char* path, char** out_text) {
    FILE* f = fopen(path, "rb");
    if (!f) return 0;
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return 0;
    }
    long size = ftell(f);
    if (size < 0) {
        fclose(f);
        return 0;
    }
    if (fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return 0;
    }
    char* buf = malloc((size_t)size + 1);
    if (!buf) {
        fclose(f);
        return 0;
    }
    size_t read_n = fread(buf, 1, (size_t)size, f);
    fclose(f);
    buf[read_n] = '\0';
    *out_text = buf;
    return 1;
}

static const char* skip_ws(const char* p) {
    while (*p && isspace((unsigned char)*p)) p++;
    return p;
}

static int json_get_string(const char* text, const char* key, char* out, size_t out_len) {
    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char* p = strstr(text, pattern);
    if (!p) return 0;
    p += strlen(pattern);
    p = strchr(p, ':');
    if (!p) return 0;
    p++;
    p = skip_ws(p);
    if (*p != '"') return 0;
    p++;
    size_t i = 0;
    while (*p && *p != '"' && i + 1 < out_len) {
        if (*p == '\\' && p[1]) p++;
        out[i++] = *p++;
    }
    out[i] = '\0';
    return i > 0;
}

static int json_get_int(const char* text, const char* key, int* out) {
    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char* p = strstr(text, pattern);
    if (!p) return 0;
    p += strlen(pattern);
    p = strchr(p, ':');
    if (!p) return 0;
    p++;
    p = skip_ws(p);
    *out = atoi(p);
    return 1;
}

static int json_get_bool(const char* text, const char* key, int* out) {
    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char* p = strstr(text, pattern);
    if (!p) return 0;
    p += strlen(pattern);
    p = strchr(p, ':');
    if (!p) return 0;
    p++;
    p = skip_ws(p);
    if (strncmp(p, "true", 4) == 0) {
        *out = 1;
        return 1;
    }
    if (strncmp(p, "false", 5) == 0) {
        *out = 0;
        return 1;
    }
    return 0;
}

int app_expand_path(const char* input, char* out, size_t out_len) {
    if (!input || !out || out_len == 0) return 0;
    if (input[0] == '~' && input[1] == '/') {
        const char* home = getenv("HOME");
        if (!home) return 0;
        return snprintf(out, out_len, "%s/%s", home, input + 2) < (int)out_len;
    }
    return snprintf(out, out_len, "%s", input) < (int)out_len;
}

void app_default_config(AppConfig* config) {
    memset(config, 0, sizeof(*config));
    config->scale = 3;
    config->padding_top = 2;
    config->padding_left = 8;
    config->clear_screen = 1;
    config->main_scale = 3;
    config->bg_scale = 1;
    config->show_background = 1;
    config->bg_requires_main = 1;
    config->persist_main_word = 1;
    config->main_use_ascii = 1;
    config->main_row_percent = 50;
    config->bg_row_from_bottom = 2;
    snprintf(config->main_color, sizeof(config->main_color), "%s", "97");
    snprintf(config->bg_color, sizeof(config->bg_color), "%s", "2;90");
    snprintf(config->ascii_font, sizeof(config->ascii_font), "%s", "block");
    config->ascii_font_file[0] = '\0';
}

int app_load_config(AppConfig* config, const char* path) {
    char resolved[1024];
    if (!app_expand_path(path, resolved, sizeof(resolved))) return 0;

    char* text = NULL;
    if (!read_file(resolved, &text)) return 0;

    json_get_int(text, "scale", &config->scale);
    json_get_int(text, "padding_top", &config->padding_top);
    json_get_int(text, "padding_left", &config->padding_left);
    json_get_bool(text, "clear_screen", &config->clear_screen);
    json_get_int(text, "main_scale", &config->main_scale);
    json_get_int(text, "bg_scale", &config->bg_scale);
    json_get_bool(text, "show_background", &config->show_background);
    json_get_bool(text, "bg_requires_main", &config->bg_requires_main);
    json_get_bool(text, "persist_main_word", &config->persist_main_word);
    json_get_bool(text, "main_use_ascii", &config->main_use_ascii);
    json_get_int(text, "main_row_percent", &config->main_row_percent);
    json_get_int(text, "bg_row_from_bottom", &config->bg_row_from_bottom);
    json_get_string(text, "main_color", config->main_color, sizeof(config->main_color));
    json_get_string(text, "bg_color", config->bg_color, sizeof(config->bg_color));
    json_get_string(text, "ascii_font", config->ascii_font, sizeof(config->ascii_font));
    json_get_string(text, "ascii_font_file", config->ascii_font_file, sizeof(config->ascii_font_file));
    json_get_string(text, "fetch_url_template", config->fetch_url_template, sizeof(config->fetch_url_template));
    if (config->main_scale <= 0) config->main_scale = config->scale > 0 ? config->scale : 3;
    if (config->bg_scale <= 0) config->bg_scale = 1;
    if (config->main_row_percent < 0) config->main_row_percent = 0;
    if (config->main_row_percent > 100) config->main_row_percent = 100;
    if (config->bg_row_from_bottom < 1) config->bg_row_from_bottom = 1;
    if (config->fetch_url_template[0] == '\0') {
        snprintf(config->fetch_url_template, sizeof(config->fetch_url_template), "%s", "http://localhost:6941");
    }
    free(text);
    return 1;
}

static void trim_line(char* s) {
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r')) s[--n] = '\0';
}

typedef enum {
    PLAYER_NONE = 0,
    PLAYER_MAC_SPOTIFY,
    PLAYER_MAC_MUSIC,
    PLAYER_MAC_NOWPLAYING_CLI,
    PLAYER_LINUX_PLAYERCTL_ALL,
    PLAYER_LINUX_PLAYERCTL_DEFAULT,
    PLAYER_WINDOWS_GSMTC
} PlayerSource;

static PlayerSource g_player_source = PLAYER_NONE;

static int parse_song_artist_line(const char* line, TrackInfo* track) {
    char buf[600];
    snprintf(buf, sizeof(buf), "%s", line);
    trim_line(buf);
    if (buf[0] == '\0') return 0;

    char* sep = strstr(buf, "|||");
    if (!sep) return 0;
    *sep = '\0';
    const char* song = buf;
    const char* artist = sep + 3;
    if (song[0] == '\0') return 0;

    snprintf(track->song, sizeof(track->song), "%s", song);
    snprintf(track->artist, sizeof(track->artist), "%s", artist);
    return 1;
}

#if defined(__linux__)
static int parse_status_artist_song_line(const char* line, TrackInfo* track) {
    char buf[600];
    snprintf(buf, sizeof(buf), "%s", line);
    trim_line(buf);
    if (buf[0] == '\0') return 0;

    char* first = strstr(buf, "|||");
    if (!first) return 0;
    *first = '\0';
    char* second = strstr(first + 3, "|||");
    if (!second) return 0;
    *second = '\0';

    const char* status = buf;
    const char* artist = first + 3;
    const char* song = second + 3;
    if (song[0] == '\0') return 0;
    if (strcasecmp(status, "playing") != 0) return 0;

    snprintf(track->song, sizeof(track->song), "%s", song);
    snprintf(track->artist, sizeof(track->artist), "%s", artist);
    return 1;
}
#endif

static int run_probe(const char* cmd, int (*parser)(const char*, TrackInfo*), TrackInfo* track) {
    FILE* p = popen(cmd, "r");
    if (!p) return 0;

    char line[600];
    while (fgets(line, sizeof(line), p)) {
        if (parser(line, track)) {
            pclose(p);
            return 1;
        }
    }
    pclose(p);
    return 0;
}

int app_get_now_playing(TrackInfo* track) {
#if defined(__APPLE__)
    const char* probes[] = {
        "osascript -l JavaScript -e 'try { var s = Application(\"Spotify\"); if (s.running() && s.playerState() === \"playing\") { s.currentTrack().name() + \"|||\" + s.currentTrack().artist(); } } catch(e) { \"\"; }' 2>/dev/null",
        "osascript -l JavaScript -e 'try { var m = Application(\"Music\"); if (m.running() && m.playerState() === \"playing\") { m.currentTrack().name() + \"|||\" + m.currentTrack().artist(); } } catch(e) { \"\"; }' 2>/dev/null",
        "nowplaying-cli get title artist 2>/dev/null | awk -F '\\t' 'NF>=2 {print $1\"|||\"$2}'"
    };
    size_t probe_count = sizeof(probes) / sizeof(probes[0]);
    for (size_t i = 0; i < probe_count; i++) {
        if (run_probe(probes[i], parse_song_artist_line, track)) {
            g_player_source = (i == 0) ? PLAYER_MAC_SPOTIFY :
                (i == 1) ? PLAYER_MAC_MUSIC : PLAYER_MAC_NOWPLAYING_CLI;
            return 1;
        }
    }
#elif defined(__linux__)
    const char* probes[] = {
        "playerctl --all-players metadata --format '{{status}}|||{{artist}}|||{{title}}' 2>/dev/null",
        "playerctl metadata --format '{{status}}|||{{artist}}|||{{title}}' 2>/dev/null"
    };
    size_t probe_count = sizeof(probes) / sizeof(probes[0]);
    for (size_t i = 0; i < probe_count; i++) {
        if (run_probe(probes[i], parse_status_artist_song_line, track)) {
            g_player_source = (i == 0) ? PLAYER_LINUX_PLAYERCTL_ALL : PLAYER_LINUX_PLAYERCTL_DEFAULT;
            return 1;
        }
    }
#elif defined(_WIN32)
    const char* probe =
        "powershell -NoProfile -Command \"try { Add-Type -AssemblyName System.Runtime.WindowsRuntime | Out-Null; $null = [Windows.Media.Control.GlobalSystemMediaTransportControlsSessionManager,Windows.Media.Control,ContentType=WindowsRuntime]; $m = [Windows.Media.Control.GlobalSystemMediaTransportControlsSessionManager]::RequestAsync().GetAwaiter().GetResult(); $s = $m.GetCurrentSession(); if ($s -ne $null) { $i = $s.TryGetMediaPropertiesAsync().GetAwaiter().GetResult(); if ($i.Title) { Write-Output ($i.Title + '|||' + $i.Artist) } } } catch {}\"";
    if (run_probe(probe, parse_song_artist_line, track)) {
        g_player_source = PLAYER_WINDOWS_GSMTC;
        return 1;
    }
#endif
    return 0;
}

static int run_position_probe(const char* cmd, double* seconds, int* is_playing) {
    FILE* p = popen(cmd, "r");
    if (!p) return 0;
    char line[128] = {0};
    if (!fgets(line, sizeof(line), p)) {
        pclose(p);
        return 0;
    }
    pclose(p);
    trim_line(line);
    if (line[0] == '\0') return 0;

    char* sep = strstr(line, "|||");
    if (!sep) return 0;
    *sep = '\0';
    *seconds = atof(line);
    *is_playing = atoi(sep + 3) ? 1 : 0;
    if (*seconds < 0.0) *seconds = 0.0;
    return 1;
}

int app_get_now_playing_position(double* seconds, int* is_playing) {
    if (!seconds || !is_playing) return 0;
    const char* cmd = NULL;

    switch (g_player_source) {
        case PLAYER_MAC_SPOTIFY:
            cmd = "osascript -l JavaScript -e 'try { var s = Application(\"Spotify\"); if (s.running()) { var playing = (s.playerState() === \"playing\") ? 1 : 0; (s.playerPosition()) + \"|||\" + playing; } else { \"\"; } } catch(e) { \"\"; }' 2>/dev/null";
            break;
        case PLAYER_MAC_MUSIC:
            cmd = "osascript -l JavaScript -e 'try { var m = Application(\"Music\"); if (m.running()) { var playing = (m.playerState() === \"playing\") ? 1 : 0; (m.playerPosition()) + \"|||\" + playing; } else { \"\"; } } catch(e) { \"\"; }' 2>/dev/null";
            break;
        case PLAYER_LINUX_PLAYERCTL_ALL:
            cmd = "playerctl --all-players metadata --format '{{position}}|||{{status}}' 2>/dev/null | awk -F '\\|\\|\\|' 'tolower($2) ~ /playing|paused/ { p=(tolower($2)==\"playing\"?1:0); print $1\"|||\"p; exit }'";
            break;
        case PLAYER_LINUX_PLAYERCTL_DEFAULT:
            cmd = "playerctl metadata --format '{{position}}|||{{status}}' 2>/dev/null | awk -F '\\|\\|\\|' '{ p=(tolower($2)==\"playing\"?1:0); print $1\"|||\"p; }'";
            break;
        case PLAYER_WINDOWS_GSMTC:
            cmd = "powershell -NoProfile -Command \"try { Add-Type -AssemblyName System.Runtime.WindowsRuntime | Out-Null; $null = [Windows.Media.Control.GlobalSystemMediaTransportControlsSessionManager,Windows.Media.Control,ContentType=WindowsRuntime]; $m = [Windows.Media.Control.GlobalSystemMediaTransportControlsSessionManager]::RequestAsync().GetAwaiter().GetResult(); $s = $m.GetCurrentSession(); if ($s -ne $null) { $p = $s.GetTimelineProperties(); $i = $s.GetPlaybackInfo(); $play = 0; if ($i.PlaybackStatus -eq 4) { $play = 1 }; Write-Output ($p.Position.TotalSeconds.ToString() + '|||' + $play.ToString()) } } catch {}\"";
            break;
        default:
            return 0;
    }

    return run_position_probe(cmd, seconds, is_playing);
}


static void normalize_ascii(const char* in, char* out, size_t out_len) {
    size_t j = 0;
    for (size_t i = 0; in[i] != '\0' && j + 1 < out_len; i++) {
        unsigned char c = (unsigned char)in[i];
        if (isalnum(c)) {
            out[j++] = (char)tolower(c);
        }
    }
    out[j] = '\0';
}

int app_find_offline_ttml(const char* db_path, const TrackInfo* track, char* out_path, size_t out_len) {
    char resolved[1024];
    if (!app_expand_path(db_path, resolved, sizeof(resolved))) return 0;

    char* text = NULL;
    if (!read_file(resolved, &text)) return 0;

    char target_song[256];
    char target_artist[256];
    normalize_ascii(track->song, target_song, sizeof(target_song));
    normalize_ascii(track->artist, target_artist, sizeof(target_artist));

    int found = 0;
    const char* p = text;
    while ((p = strchr(p, '{')) != NULL) {
        const char* q = strchr(p, '}');
        if (!q) break;
        size_t obj_len = (size_t)(q - p + 1);
        char* obj = malloc(obj_len + 1);
        if (!obj) break;
        memcpy(obj, p, obj_len);
        obj[obj_len] = '\0';

        char songname[256] = {0};
        char artist[256] = {0};
        char path[1024] = {0};
        json_get_string(obj, "songname", songname, sizeof(songname));
        json_get_string(obj, "artist", artist, sizeof(artist));
        json_get_string(obj, "path", path, sizeof(path));

        char n_song[256];
        char n_artist[256];
        normalize_ascii(songname, n_song, sizeof(n_song));
        normalize_ascii(artist, n_artist, sizeof(n_artist));

        if (path[0] != '\0' && strcmp(n_song, target_song) == 0 &&
            (target_artist[0] == '\0' || n_artist[0] == '\0' || strcmp(n_artist, target_artist) == 0)) {
            found = app_expand_path(path, out_path, out_len);
            free(obj);
            break;
        }
        free(obj);
        p = q + 1;
    }

    free(text);
    if (!found) return 0;
    return access(out_path, R_OK) == 0;
}

static void url_encode(const char* in, char* out, size_t out_len) {
    const char* hex = "0123456789ABCDEF";
    size_t j = 0;
    for (size_t i = 0; in[i] != '\0' && j + 1 < out_len; i++) {
        unsigned char c = (unsigned char)in[i];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
            out[j++] = (char)c;
        } else if (j + 3 < out_len) {
            out[j++] = '%';
            out[j++] = hex[(c >> 4) & 0xF];
            out[j++] = hex[c & 0xF];
        }
    }
    out[j] = '\0';
}

static int render_template_url(const char* tpl, const TrackInfo* track, char* out, size_t out_len) {
    char song_enc[1024];
    char artist_enc[1024];
    url_encode(track->song, song_enc, sizeof(song_enc));
    url_encode(track->artist, artist_enc, sizeof(artist_enc));

    size_t j = 0;
    for (size_t i = 0; tpl[i] != '\0' && j + 1 < out_len; i++) {
        if (strncmp(&tpl[i], "{song}", 6) == 0) {
            size_t n = strlen(song_enc);
            if (j + n >= out_len) return 0;
            memcpy(&out[j], song_enc, n);
            j += n;
            i += 5;
        } else if (strncmp(&tpl[i], "{artist}", 8) == 0) {
            size_t n = strlen(artist_enc);
            if (j + n >= out_len) return 0;
            memcpy(&out[j], artist_enc, n);
            j += n;
            i += 7;
        } else {
            out[j++] = tpl[i];
        }
    }
    out[j] = '\0';
    return 1;
}

int app_fetch_online_ttml(const AppConfig* config, const TrackInfo* track, char* out_path, size_t out_len) {
    if (config->fetch_url_template[0] == '\0') return 0;

    char url[2048];
    if (!render_template_url(config->fetch_url_template, track, url, sizeof(url))) return 0;

    char tmp_template[] = "/tmp/nil_ttml_XXXXXX";
    int fd = mkstemp(tmp_template);
    if (fd < 0) return 0;
    close(fd);

    char cmd[4096];
    snprintf(cmd, sizeof(cmd), "curl -fsSL --max-time 12 \"%s\" -o \"%s\"", url, tmp_template);
    int rc = system(cmd);
    if (rc != 0) {
        unlink(tmp_template);
        return 0;
    }

    FILE* f = fopen(tmp_template, "rb");
    if (!f) {
        unlink(tmp_template);
        return 0;
    }
    char probe[32] = {0};
    size_t n = fread(probe, 1, sizeof(probe) - 1, f);
    fclose(f);
    probe[n] = '\0';
    if (strstr(probe, "<tt") == NULL && strstr(probe, "<?xml") == NULL) {
        unlink(tmp_template);
        return 0;
    }

    return snprintf(out_path, out_len, "%s", tmp_template) < (int)out_len;
}
