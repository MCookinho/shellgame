/******************************************************************************
 *                                ENDURO
 *         Race through traffic and survive the open road!
 ******************************************************************************/
#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include "config.h"

#define ROAD_W 14
#define ROAD_H 20
#define MAX_ENEMIES 12
#define PASS_PER_DAY 15
#define MAX_DAY 4

typedef struct { int y, x, active; } Enemy;

static int player_x;
static Enemy enemies[MAX_ENEMIES];
static int score, day, over, frame;
static int road_offset;
static int spawn_counter;

static void init_game(void) {
    player_x = ROAD_W / 2 - 1;
    score = 0; day = 1; over = 0; frame = 0;
    road_offset = 0; spawn_counter = 0;
    for (int i = 0; i < MAX_ENEMIES; i++) enemies[i].active = 0;
}

static void spawn_enemy(void) {
    for (int i = 0; i < MAX_ENEMIES; i++)
        if (!enemies[i].active) {
            enemies[i] = (Enemy){ 0, rand() % (ROAD_W - 3), 1 };
            break;
        }
}

static int get_speed(void) { return 12 - day * 2; }
static int get_spawn_rate(void) { return 25 - day * 4; }

static void update(void) {
    int speed = get_speed();
    if (frame % speed == 0) {
        road_offset = (road_offset + 1) % 4;
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (!enemies[i].active) continue;
            enemies[i].y++;
            if (enemies[i].y >= ROAD_H) {
                enemies[i].active = 0;
                score++;
            }
        }
    }

    int rate = get_spawn_rate();
    if (rate < 5) rate = 5;
    spawn_counter++;
    if (spawn_counter >= rate) {
        spawn_counter = 0;
        spawn_enemy();
    }

    // Collision
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;
        if (enemies[i].y >= ROAD_H - 3 && enemies[i].y <= ROAD_H - 2 &&
            enemies[i].x < player_x + 3 && enemies[i].x + 3 > player_x) {
            over = 1;
            return;
        }
    }

    // Day progression
    if (score > 0 && score % PASS_PER_DAY == 0 && day < MAX_DAY) {
        day++;
    }
}

int main(void) {
    srand(time(NULL));
    read_config();
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    start_color();

    init_theme_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_theme_pair(2, COLOR_YELLOW, COLOR_BLACK);
    init_theme_pair(3, COLOR_RED, COLOR_BLACK);
    init_theme_pair(4, COLOR_WHITE, COLOR_BLACK);
    init_theme_pair(5, COLOR_CYAN, COLOR_BLACK);
    init_theme_pair(6, COLOR_BLUE, COLOR_BLACK);
    apply_theme_bg();

    int play_again = 1;
    while (play_again) {
        init_game();

        int ch;
        int oy = (LINES - ROAD_H - 4) / 2;
        int ox = (COLS - ROAD_W - 4) / 2;

        while (!over) {
            ch = getch();
            if (ch == 'q') { over = 1; play_again = 0; break; }
            if ((ch == KEY_LEFT || ch == 'a') && player_x > 0) player_x--;
            if ((ch == KEY_RIGHT || ch == 'd') && player_x < ROAD_W - 3) player_x++;

            update();

            erase();

            int gx = ox - 4;

            // Grass
            attron(COLOR_PAIR(1));
            for (int y = 0; y < ROAD_H + 2; y++) {
                for (int g = 0; g < 4; g++) {
                    mvaddch(oy + y, gx + g, '\'');
                    mvaddch(oy + y, gx + 4 + ROAD_W + 4 + g, '\'');
                }
            }
            attroff(COLOR_PAIR(1));

            // Road borders
            attron(COLOR_PAIR(4));
            for (int y = 0; y < ROAD_H + 2; y++) {
                mvaddch(oy + y, gx + 4, '|');
                mvaddch(oy + y, gx + 4 + ROAD_W + 1, '|');
            }
            attroff(COLOR_PAIR(4));

            // Road surface
            attron(COLOR_PAIR(6));
            for (int y = 0; y < ROAD_H + 2; y++) {
                for (int x = 0; x < ROAD_W; x++) {
                    int ch = ' ';
                    // Road markings (dashed center line)
                    if (x == ROAD_W / 2 || x == ROAD_W / 2 - 1) {
                        int ry = (road_offset + y) % 6;
                        ch = (ry < 3) ? ' ' : '|';
                    }
                    mvaddch(oy + y, gx + 5 + x, ch);
                }
            }
            attroff(COLOR_PAIR(6));

            // Enemies
            attron(COLOR_PAIR(3) | A_BOLD);
            for (int i = 0; i < MAX_ENEMIES; i++) {
                if (!enemies[i].active) continue;
                if (enemies[i].y < 0 || enemies[i].y >= ROAD_H) continue;
                int ey = oy + 1 + enemies[i].y;
                int ex = gx + 5 + enemies[i].x;
                if (day >= 4 && (frame / 10) % 2 == 0) {
                    attron(A_REVERSE);
                    mvaddstr(ey, ex, "###");
                    attroff(A_REVERSE);
                } else {
                    mvaddstr(ey, ex, "[#]");
                }
            }
            attroff(COLOR_PAIR(3) | A_BOLD);

            // Player car
            int py = oy + ROAD_H - 2;
            int px = gx + 5 + player_x;
            attron(COLOR_PAIR(2) | A_BOLD);
            if (day >= 3 && (frame / 8) % 2 == 0) {
                attron(A_REVERSE);
                mvaddstr(py, px, "[I]");
                attroff(A_REVERSE);
            } else {
                mvaddstr(py, px, "[I]");
            }
            attroff(COLOR_PAIR(2) | A_BOLD);

            // HUD
            attron(COLOR_PAIR(5));
            char buf[64];
            snprintf(buf, sizeof(buf), _("Score: %d  Day: %d/%d", "Pontos: %d  Dia: %d/%d"),
                     score, day, MAX_DAY);
            mvaddstr(oy + ROAD_H + 3, gx, buf);
            mvaddstr(oy + ROAD_H + 4, gx,
                     _("<- ->: move | Q: quit", "<- ->: mover | Q: sair"));
            attroff(COLOR_PAIR(5));

            refresh();
            usleep(50000);
            frame++;
        }

        if (play_again) {
            erase();
            int cx = COLS / 2 - 12;
            attron(COLOR_PAIR(2) | A_BOLD);
            mvaddstr(oy + ROAD_H / 2, cx, _("RACE OVER!", "CORRIDA ENCERRADA!"));
            attroff(COLOR_PAIR(2) | A_BOLD);
            attron(COLOR_PAIR(5));
            char buf[64];
            snprintf(buf, sizeof(buf), _("Score: %d  Day: %d", "Pontuacao: %d  Dia: %d"), score, day);
            mvaddstr(oy + ROAD_H / 2 + 2, cx, buf);
            mvaddstr(oy + ROAD_H / 2 + 4, cx,
                     _("Enter: play again | Q: quit", "Enter: jogar novamente | Q: sair"));
            attroff(COLOR_PAIR(5));
            refresh();
            nodelay(stdscr, FALSE);
            while (1) {
                ch = getch();
                if (ch == 'q') { play_again = 0; break; }
                if (ch == '\n') { play_again = 1; break; }
            }
            nodelay(stdscr, TRUE);
        }
    }

    endwin();
    return 0;
}
