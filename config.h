/******************************************************************************
 *                           SHELL GAMES - Config
 *            Shared configuration for language and theme support
 ******************************************************************************/
#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#define CFG_DIR  ".config/shellgames"
#define CFG_FILE "config"

typedef enum { LANG_EN, LANG_PT } Lang;
typedef enum { THEME_DARK, THEME_LIGHT, THEME_RETRO } Theme;

static Lang cfg_lang = LANG_EN;
static Theme cfg_theme = THEME_DARK;
static int cfg_speed = 0;
static int cfg_hints = 1;

static const char *cfg_path(void) {
    static char path[512];
    const char *home = getenv("HOME");
    if (!home) home = ".";
    snprintf(path, sizeof(path), "%s/%s/%s", home, CFG_DIR, CFG_FILE);
    return path;
}

static void read_config(void) {
    const char *path = cfg_path();
    FILE *f = fopen(path, "r");
    if (!f) return;
    char line[64];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = 0;
        if (!strncmp(line, "lang=", 5)) {
            if (!strcmp(line + 5, "pt")) cfg_lang = LANG_PT;
            else cfg_lang = LANG_EN;
        }
        if (!strncmp(line, "theme=", 6)) {
            if (!strcmp(line + 6, "light")) cfg_theme = THEME_LIGHT;
            else if (!strcmp(line + 6, "retro")) cfg_theme = THEME_RETRO;
            else cfg_theme = THEME_DARK;
        }
        if (!strncmp(line, "speed=", 6)) {
            cfg_speed = !strcmp(line + 6, "fast") ? 1 : 0;
        }
        if (!strncmp(line, "hints=", 6)) {
            cfg_hints = !strcmp(line + 6, "hide") ? 0 : 1;
        }
    }
    fclose(f);
}

static __attribute__((unused)) void write_config(void) {
    const char *path = cfg_path();
    char dir[512];
    snprintf(dir, sizeof(dir), "%s/%s", getenv("HOME") ? getenv("HOME") : ".", CFG_DIR);
    mkdir(dir, 0755);
    FILE *f = fopen(path, "w");
    if (!f) return;
    fprintf(f, "lang=%s\n", cfg_lang == LANG_PT ? "pt" : "en");
    fprintf(f, "theme=%s\n", cfg_theme == THEME_DARK ? "dark" : cfg_theme == THEME_LIGHT ? "light" : "retro");
    fprintf(f, "speed=%s\n", cfg_speed ? "fast" : "normal");
    fprintf(f, "hints=%s\n", cfg_hints ? "show" : "hide");
    fclose(f);
}

#define _(EN, PT) (cfg_lang == LANG_PT ? (PT) : (EN))

static int bg_color(void) {
    if (cfg_theme == THEME_LIGHT) return COLOR_WHITE;
    return COLOR_BLACK;
}

static int fg_color(int default_fg) {
    if (cfg_theme == THEME_LIGHT) {
        if (default_fg == COLOR_BLACK) return COLOR_WHITE;
        if (default_fg == COLOR_WHITE) return COLOR_BLACK;
        return default_fg;
    }
    if (cfg_theme == THEME_RETRO && default_fg != COLOR_BLACK && default_fg != COLOR_WHITE)
        return COLOR_GREEN;
    return default_fg;
}

static void init_theme_pair(short id, short fg, short bg) {
    if (bg == COLOR_BLACK) bg = bg_color();
    else if (cfg_theme == THEME_LIGHT && bg == COLOR_WHITE) bg = COLOR_BLACK;
    fg = fg_color(fg);
    if (cfg_theme == THEME_RETRO && fg != COLOR_BLACK) fg = COLOR_GREEN;
    if (cfg_theme == THEME_RETRO && bg != COLOR_BLACK) bg = COLOR_BLACK;
    init_pair(id, fg, bg);
}

static void apply_theme_bg(void) {
    // Set the window default background to match the theme
    short bg = bg_color();
    short fg = (cfg_theme == THEME_DARK) ? COLOR_WHITE :
               (cfg_theme == THEME_LIGHT) ? COLOR_BLACK : COLOR_GREEN;
    // Use pair 99 as the global background pair
    init_pair(99, fg, bg);
    bkgd(COLOR_PAIR(99));
}

#endif
