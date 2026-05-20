/******************************************************************************
 *                              GALAGA
 *          Classic arcade space shooter - waves of alien attack!
 ******************************************************************************/
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <locale.h>
#include <math.h>
#include "config.h"

#define AREA_W 50
#define AREA_H 22
#define GRID_COLS 8
#define GRID_ROWS 4
#define MAX_ALIENS 32
#define MAX_PB 4
#define MAX_EB 30
#define PLAYER_Y (AREA_H - 2)

typedef struct {
    int x, y, type, alive, diving;
    int phase, sx, sy;
    int btimer;
} Alien;

typedef struct { int x, y, active; } Bullet;

static Alien aliens[MAX_ALIENS];
static Bullet pb[MAX_PB], eb[MAX_EB];
static int px, pvx, lives, score, stage, total_aliens;
static int game_over, won, shoot_cd;
static int formation_dir, formation_x, formation_y;
static int diver_count, stage_delay;
static int combo;
static const char *alien_ch[3] = {"(O)", "\\V/", "/W\\"};

static void spawn_bullet(void) {
    for (int i = 0; i < MAX_PB; i++) {
        if (!pb[i].active) {
            pb[i].x = px + 1;
            pb[i].y = PLAYER_Y - 1;
            pb[i].active = 1;
            return;
        }
    }
}

static void spawn_eb(int x, int y) {
    for (int i = 0; i < MAX_EB; i++) {
        if (!eb[i].active) {
            eb[i].x = x + 1;
            eb[i].y = y + 1;
            eb[i].active = 1;
            return;
        }
    }
}

static void start_dive(int idx) {
    Alien *a = &aliens[idx];
    a->diving = 1;
    a->phase = 0;
    a->sx = a->x;
    a->sy = a->y;
    a->btimer = 15 + rand() % 25;
    diver_count++;
}

static void init_stage(void) {
    stage++;
    formation_x = 5;
    formation_y = 3;
    formation_dir = 1;
    diver_count = 0;
    stage_delay = 0;

    total_aliens = GRID_COLS * GRID_ROWS;
    int idx = 0;
    for (int r = 0; r < GRID_ROWS; r++) {
        for (int c = 0; c < GRID_COLS; c++) {
            aliens[idx].x = formation_x + c * 5;
            aliens[idx].y = formation_y + r * 3;
            aliens[idx].type = (r == 0) ? 0 : (r == 1 ? 1 : 2);
            aliens[idx].alive = 1;
            aliens[idx].diving = 0;
            aliens[idx].phase = 0;
            aliens[idx].btimer = 0;
            idx++;
        }
    }

    for (int i = 0; i < MAX_EB; i++) eb[i].active = 0;
    for (int i = 0; i < MAX_EB; i++) eb[i].active = 0;

    px = AREA_W / 2;
    pvx = 0;
    shoot_cd = 0;
}

static void new_game(void) {
    lives = 3;
    score = 0;
    stage = 0;
    combo = 0;
    game_over = 0;
    won = 0;
    init_stage();
}

static void update_formation(void) {
    formation_x += formation_dir;
    if (formation_x < 2 || formation_x > AREA_W - GRID_COLS * 5 - 4)
        formation_dir = -formation_dir;

    for (int i = 0; i < total_aliens; i++) {
        if (!aliens[i].alive || aliens[i].diving) continue;
        int r = i / GRID_COLS, c = i % GRID_COLS;
        aliens[i].x = formation_x + c * 5;
        aliens[i].y = formation_y + r * 3;
    }
}

static void update_dives(void) {
    if (diver_count >= 1 + stage / 3) return;
    if (rand() % 120 != 0) return;

    int candidates[MAX_ALIENS], n = 0;
    for (int i = 0; i < total_aliens; i++)
        if (aliens[i].alive && !aliens[i].diving) candidates[n++] = i;
    if (n == 0) return;
    start_dive(candidates[rand() % n]);
}

static void update_divers(void) {
    for (int i = 0; i < total_aliens; i++) {
        if (!aliens[i].alive || !aliens[i].diving) continue;
        Alien *a = &aliens[i];
        a->phase++;
        int p = a->phase;

        if (p < 30) {
            a->x = a->sx + p;
            a->y = a->sy + p / 2;
        } else if (p < 90) {
            float t = (p - 30) / 60.0 * 3.14159;
            a->x = a->sx + 30 - (int)(sin(t) * 20);
            a->y = a->sy + 15 + (int)(sin(t * 2) * 8);
        } else if (p < 130) {
            float t = (p - 90) / 40.0 * 3.14159;
            a->x = a->sx + 10 + (int)(cos(t) * 15);
            a->y = a->sy + 5 + (int)((1 - cos(t)) * 10);
        } else {
            a->diving = 0;
            a->x = a->sx;
            a->y = a->sy;
            diver_count--;
            continue;
        }

        if (a->x < 1) a->x = 1;
        if (a->x >= AREA_W - 3) a->x = AREA_W - 4;
        if (a->y < 1) a->y = 1;
        if (a->y >= AREA_H - 1) a->y = AREA_H - 1;

        /* Shoot during dive */
        a->btimer--;
        if (a->btimer <= 0) {
            a->btimer = 25 + rand() % 30 - stage;
            if (a->btimer < 10) a->btimer = 10;
            spawn_eb(a->x, a->y);
        }

        /* Collision with player */
        if (abs(a->x - px) < 3 && abs(a->y - PLAYER_Y) < 2) {
            lives--;
            if (lives <= 0) { game_over = 1; return; }
            for (int j = 0; j < MAX_PB; j++) pb[j].active = 0;
            for (int j = 0; j < MAX_EB; j++) eb[j].active = 0;
            a->diving = 0;
            a->x = a->sx;
            a->y = a->sy;
            diver_count--;
        }
    }
}

static void update_bullets(void) {
    for (int i = 0; i < MAX_PB; i++) {
        if (!pb[i].active) continue;
        pb[i].y -= 2;
        if (pb[i].y < 0) { pb[i].active = 0; continue; }

        for (int j = 0; j < total_aliens; j++) {
            if (!aliens[j].alive) continue;
            Alien *a = &aliens[j];
            if (abs(pb[i].x - a->x - 1) < 2 && abs(pb[i].y - a->y) < 2) {
                pb[i].active = 0;
                a->alive = 0;
                if (a->diving) diver_count--;
                int pts = (a->type == 0) ? 150 : (a->type == 1 ? 80 : 50);
                combo++;
                score += pts + combo * 5;
                break;
            }
        }
    }

    for (int i = 0; i < MAX_EB; i++) {
        if (!eb[i].active) continue;
        eb[i].y += 1;
        if (eb[i].y > AREA_H) { eb[i].active = 0; continue; }
        if (abs(eb[i].x - px - 1) < 2 && abs(eb[i].y - PLAYER_Y) < 2) {
            eb[i].active = 0;
            lives--;
            combo = 0;
            if (lives <= 0) { game_over = 1; return; }
            for (int j = 0; j < MAX_PB; j++) pb[j].active = 0;
        }
    }
}

static void draw(int ox, int oy) {
    erase();
    char buf[64];

    /* HUD */
    attron(COLOR_PAIR(5) | A_BOLD);
    mvaddstr(oy, ox, _("GALAGA", "GALAGA"));
    attroff(COLOR_PAIR(5) | A_BOLD);

    snprintf(buf, 64, _("Stage: %d  Score: %d  Lives: %d  Combo: x%d",
                         "Fase: %d  Pontos: %d  Vidas: %d  Combo: x%d"),
             stage, score, lives, combo);
    attron(COLOR_PAIR(2));
    mvaddstr(oy + 1, ox, buf);
    attroff(COLOR_PAIR(2));

    /* Play area border */
    int ay = oy + 3;
    int ax = ox;
    for (int c = 0; c < AREA_W; c++) {
        attron(COLOR_PAIR(6) | A_DIM);
        mvaddch(ay, ax + c, '-');
        mvaddch(ay + AREA_H + 1, ax + c, '-');
        attroff(COLOR_PAIR(6) | A_DIM);
    }
    for (int r = 0; r < AREA_H + 2; r++) {
        attron(COLOR_PAIR(6) | A_DIM);
        mvaddch(ay + r, ax, '|');
        mvaddch(ay + r, ax + AREA_W - 1, '|');
        attroff(COLOR_PAIR(6) | A_DIM);
    }

    /* Aliens in formation */
    for (int i = 0; i < total_aliens; i++) {
        if (!aliens[i].alive || aliens[i].diving) continue;
        int col = (aliens[i].type == 0) ? 4 : (aliens[i].type == 1 ? 2 : 5);
        attron(COLOR_PAIR(col) | A_BOLD);
        mvaddstr(ay + aliens[i].y, ax + aliens[i].x, alien_ch[aliens[i].type]);
        attroff(COLOR_PAIR(col) | A_BOLD);
    }

    /* Diving aliens */
    for (int i = 0; i < total_aliens; i++) {
        if (!aliens[i].alive || !aliens[i].diving) continue;
        int col = (aliens[i].type == 0) ? 4 : (aliens[i].type == 1 ? 2 : 5);
        attron(COLOR_PAIR(col) | A_BOLD);
        mvaddstr(ay + aliens[i].y, ax + aliens[i].x, alien_ch[aliens[i].type]);
        attroff(COLOR_PAIR(col) | A_BOLD);
    }

    /* Player bullets */
    for (int i = 0; i < MAX_PB; i++) {
        if (!pb[i].active) continue;
        attron(COLOR_PAIR(3) | A_BOLD);
        mvaddch(ay + pb[i].y, ax + pb[i].x, '!');
        attroff(COLOR_PAIR(3) | A_BOLD);
    }

    /* Enemy bullets */
    for (int i = 0; i < MAX_EB; i++) {
        if (!eb[i].active) continue;
        attron(COLOR_PAIR(4) | A_BOLD);
        mvaddch(ay + eb[i].y, ax + eb[i].x, '*');
        attroff(COLOR_PAIR(4) | A_BOLD);
    }

    /* Player ship */
    attron(COLOR_PAIR(3) | A_BOLD);
    mvaddstr(ay + PLAYER_Y, ax + px, "/-\\");
    attroff(COLOR_PAIR(3) | A_BOLD);

    /* Game over */
    if (game_over) {
        attron(COLOR_PAIR(4) | A_BOLD);
        mvaddstr(ay + AREA_H / 2 - 2, ax + AREA_W / 2 - 6,
                 _("GAME OVER", "FIM DE JOGO"));
        attroff(COLOR_PAIR(4) | A_BOLD);
        snprintf(buf, 64, _("Final Score: %d", "Pontuacao: %d"), score);
        attron(COLOR_PAIR(2));
        mvaddstr(ay + AREA_H / 2, ax + AREA_W / 2 - 8, buf);
        attroff(COLOR_PAIR(2));
        attron(COLOR_PAIR(6) | A_DIM);
        mvaddstr(ay + AREA_H / 2 + 2, ax + AREA_W / 2 - 10,
                 _("Enter: play again  Q: quit", "Enter: jogar novamente  Q: sair"));
        attroff(COLOR_PAIR(6) | A_DIM);
    }

    /* Instructions */
    attron(COLOR_PAIR(6) | A_DIM);
    mvaddstr(oy + AREA_H + 6, ox,
             _("Arrows/WASD: move  Space: shoot  Q: quit",
               "Setas/WASD: mover  Espaco: atirar  Q: sair"));
    attroff(COLOR_PAIR(6) | A_DIM);

    refresh();
}

int main(void) {
    setlocale(LC_ALL, "");
    srand(time(NULL));
    read_config();
    initscr(); cbreak(); noecho(); curs_set(0);
    keypad(stdscr, TRUE); nodelay(stdscr, TRUE);
    start_color();

    init_theme_pair(1, COLOR_CYAN, COLOR_BLACK);
    init_theme_pair(2, COLOR_YELLOW, COLOR_BLACK);
    init_theme_pair(3, COLOR_GREEN, COLOR_BLACK);
    init_theme_pair(4, COLOR_RED, COLOR_BLACK);
    init_theme_pair(5, COLOR_MAGENTA, COLOR_BLACK);
    init_theme_pair(6, COLOR_WHITE, COLOR_BLACK);
    apply_theme_bg();

    int play = 1;
    while (play) {
        new_game();

        while (1) {
            int ox = (COLS - AREA_W) / 2 < 1 ? 1 : (COLS - AREA_W) / 2;
            int oy = (LINES - 28) / 2 < 1 ? 1 : (LINES - 28) / 2;

            if (!game_over) {
                update_formation();
                update_dives();
                update_divers();
                if (game_over) { draw(ox, oy); goto input; }
                update_bullets();
                if (game_over) { draw(ox, oy); goto input; }

                /* Check stage clear */
                int alive = 0;
                for (int i = 0; i < total_aliens; i++)
                    if (aliens[i].alive) alive++;
                if (alive == 0 && diver_count == 0) {
                    if (stage >= 10) { game_over = 1; won = 1; }
                    else init_stage();
                }
            }

input:
            draw(ox, oy);

            int ch = getch();
            if (ch == 'q') { play = 0; break; }

            if (game_over) {
                if (ch == '\n' || ch == ' ') break;
                usleep(16000);
                continue;
            }

            /* Player movement */
            pvx = 0;
            if (ch == KEY_LEFT || ch == 'a') pvx = -1;
            else if (ch == KEY_RIGHT || ch == 'd') pvx = 1;
            px += pvx;
            if (px < 1) px = 1;
            if (px > AREA_W - 4) px = AREA_W - 4;

            if (ch == ' ') {
                spawn_bullet();
            }

            usleep(33000);
        }
        if (!play) break;
    }

    endwin();
    return 0;
}
