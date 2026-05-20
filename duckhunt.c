/******************************************************************************
 *                            DUCK HUNT
 *        Aim and shoot ducks in this classic lightgun gallery!
 ******************************************************************************/
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <locale.h>
#include "config.h"

#define MAX_DUCKS 3
#define SHOTS_PER_ROUND 3
#define MAX_ROUNDS 15
#define DUCK_TIMEOUT 160
#define AREA_W 50
#define AREA_H 16

typedef struct {
    int x, y, vx, vy;
    int alive, escaped;
    int timer, flap;
    int dir;
} Duck;

static Duck ducks[MAX_DUCKS];
static int cross_x, cross_y;
static int shots, round_num, score;
static int ducks_in_round, ducks_hit;
static int game_over, round_over, round_won;
static int round_timer, state;
enum { ST_ROUND_START, ST_PLAYING, ST_ROUND_OVER, ST_GAME_OVER };

static const char *duck_r[2] = {">(\")>", ">(^)>"};
static const char *duck_l[2] = {"<(\")<", "<(^)<"};

static void new_round(void) {
    round_num++;
    if (round_num > MAX_ROUNDS) { game_over = 1; return; }
    ducks_in_round = 1 + (round_num / 3);
    if (ducks_in_round > MAX_DUCKS) ducks_in_round = MAX_DUCKS;
    shots = SHOTS_PER_ROUND;
    ducks_hit = 0;
    round_over = 0;
    round_won = 0;
    state = ST_ROUND_START;
    round_timer = 30;

    int speed = 1 + round_num / 5;
    if (speed > 3) speed = 3;
    int escape_time = DUCK_TIMEOUT - round_num * 8;
    if (escape_time < 60) escape_time = 60;

    for (int i = 0; i < MAX_DUCKS; i++) {
        ducks[i].alive = 0;
        ducks[i].escaped = 0;
    }

    for (int i = 0; i < ducks_in_round; i++) {
        int side = rand() % 3;
        if (side == 0) {
            ducks[i].x = 2;
            ducks[i].y = 2 + rand() % (AREA_H - 6);
            ducks[i].vx = speed;
            ducks[i].dir = 1;
        } else if (side == 1) {
            ducks[i].x = AREA_W - 4;
            ducks[i].y = 2 + rand() % (AREA_H - 6);
            ducks[i].vx = -speed;
            ducks[i].dir = -1;
        } else {
            ducks[i].x = 5 + rand() % (AREA_W - 10);
            ducks[i].y = 2;
            ducks[i].vx = (rand() % 2) ? speed : -speed;
            ducks[i].dir = ducks[i].vx;
        }
        ducks[i].vy = 0;
        ducks[i].alive = 1;
        ducks[i].escaped = 0;
        ducks[i].timer = escape_time + rand() % 30;
        ducks[i].flap = 0;
    }
}

static void new_game(void) {
    round_num = 0;
    score = 0;
    game_over = 0;
    cross_x = AREA_W / 2;
    cross_y = AREA_H / 2;
    new_round();
}

static void update_ducks(void) {
    for (int i = 0; i < MAX_DUCKS; i++) {
        if (!ducks[i].alive || ducks[i].escaped) continue;

        ducks[i].x += ducks[i].vx;
        int vy_range = 1 + round_num / 5;
        if (vy_range > 3) vy_range = 3;
        ducks[i].vy += (rand() % (vy_range * 2 + 1)) - vy_range;
        int vy_cap = 2 + round_num / 5;
        if (vy_cap > 5) vy_cap = 5;
        if (ducks[i].vy > vy_cap) ducks[i].vy = vy_cap;
        if (ducks[i].vy < -vy_cap) ducks[i].vy = -vy_cap;
        ducks[i].y += ducks[i].vy;
        if (ducks[i].y < 1) { ducks[i].y = 1; ducks[i].vy = 1; }
        if (ducks[i].y >= AREA_H - 2) { ducks[i].y = AREA_H - 3; ducks[i].vy = -1; }
        if (ducks[i].x < 1) { ducks[i].x = 1; ducks[i].vx = -ducks[i].vx; ducks[i].dir = 1; }
        if (ducks[i].x >= AREA_W - 4) { ducks[i].x = AREA_W - 5; ducks[i].vx = -ducks[i].vx; ducks[i].dir = -1; }

        ducks[i].dir = (ducks[i].vx > 0) ? 1 : -1;
        ducks[i].timer--;
        if (ducks[i].timer <= 0) {
            ducks[i].escaped = 1;
        }
        ducks[i].flap = (ducks[i].flap + 1) % 2;
    }
}

static void shoot(void) {
    if (shots <= 0 || round_over) return;
    shots--;

    for (int i = 0; i < MAX_DUCKS; i++) {
        if (!ducks[i].alive || ducks[i].escaped) continue;
        int dx = cross_x - ducks[i].x;
        int dy = cross_y - ducks[i].y;
        if (dx >= -2 && dx <= 4 && dy >= -2 && dy <= 2) {
            ducks[i].alive = 0;
            ducks_hit++;
            score += 100 + (round_num * 10) + (ducks_in_round * 20);
        }
    }
}

static int all_done(void) {
    for (int i = 0; i < ducks_in_round; i++)
        if (ducks[i].alive && !ducks[i].escaped) return 0;
    return 1;
}

static void draw(int ox, int oy) {
    erase();

    /* Title and stats */
    char buf[64];
    attron(COLOR_PAIR(2) | A_BOLD);
    mvaddstr(oy, ox, _("DUCK HUNT", "DUCK HUNT"));
    attroff(COLOR_PAIR(2) | A_BOLD);

    snprintf(buf, 64, _("R: %d  Score: %d  Shots: %d/%d",
                         "R: %d  Pontos: %d  Tiros: %d/%d"),
             round_num, score, shots, SHOTS_PER_ROUND);
    attron(COLOR_PAIR(5));
    mvaddstr(oy + 1, ox, buf);
    attroff(COLOR_PAIR(5));

    /* Play area border */
    int ay = oy + 3;
    int ax = ox;
    for (int c = 0; c < AREA_W; c++) {
        mvaddch(ay, ax + c, '-');
        mvaddch(ay + AREA_H + 1, ax + c, '-');
    }
    for (int r = 0; r < AREA_H + 2; r++) {
        mvaddch(ay + r, ax, '|');
        mvaddch(ay + r, ax + AREA_W - 1, '|');
    }

    /* Grass at bottom */
    attron(COLOR_PAIR(3) | A_DIM);
    for (int c = 1; c < AREA_W - 1; c++)
        mvaddch(ay + AREA_H, ax + c, '"');
    attroff(COLOR_PAIR(3) | A_DIM);

    /* Ducks */
    for (int i = 0; i < MAX_DUCKS; i++) {
        if (!ducks[i].alive || ducks[i].escaped) continue;
        int dx = ax + ducks[i].x;
        int dy = ay + ducks[i].y;
        const char **sprite = (ducks[i].dir == 1) ? duck_r : duck_l;
        attron(COLOR_PAIR(2) | A_BOLD);
        mvaddstr(dy, dx, sprite[ducks[i].flap]);
        attroff(COLOR_PAIR(2) | A_BOLD);
    }

    /* Dead ducks (falling) */
    for (int i = 0; i < MAX_DUCKS; i++) {
        if (ducks[i].alive || ducks[i].escaped) continue;
        if (ducks[i].timer < -20) continue;
        int dx = ax + ducks[i].x;
        int dy = ay + ducks[i].y + (-ducks[i].timer / 4);
        if (dy > ay + AREA_H) dy = ay + AREA_H;
        attron(COLOR_PAIR(4) | A_BOLD);
        mvaddstr(dy, dx, "*.*");
        attroff(COLOR_PAIR(4) | A_BOLD);
    }

    /* Escaped ducks fly off */
    for (int i = 0; i < MAX_DUCKS; i++) {
        if (!ducks[i].escaped) continue;
        int dx = ax + ducks[i].x + ducks[i].vx * 3;
        int dy = ay + ducks[i].y;
        if (dx > 0 && dx < AREA_W - 4 && dy > 0 && dy < AREA_H) {
            attron(COLOR_PAIR(6) | A_DIM);
            mvaddch(dy, dx, '>');
            attroff(COLOR_PAIR(6) | A_DIM);
        }
    }

    /* Crosshair */
    attron(COLOR_PAIR(4) | A_BOLD);
    int cx = ax + cross_x;
    int cy = ay + cross_y;
    mvaddch(cy, cx, '+');
    if (cy > ay + 1) mvaddch(cy - 1, cx, '|');
    if (cy < ay + AREA_H - 1) mvaddch(cy + 1, cx, '|');
    if (cx > ax + 1) mvaddch(cy, cx - 1, '-');
    if (cx < ax + AREA_W - 2) mvaddch(cy, cx + 1, '-');
    attroff(COLOR_PAIR(4) | A_BOLD);

    /* Round start message */
    if (state == ST_ROUND_START) {
        attron(COLOR_PAIR(5) | A_BOLD);
        snprintf(buf, 64, _("ROUND %d", "RODADA %d"), round_num);
        mvaddstr(ay + AREA_H / 2 - 1, ax + AREA_W / 2 - 4, buf);
        snprintf(buf, 64, _("%d duck(s)!", "%d pato(s)!"), ducks_in_round);
        mvaddstr(ay + AREA_H / 2 + 1, ax + AREA_W / 2 - 5, buf);
        attroff(COLOR_PAIR(5) | A_BOLD);
    }

    /* Round over */
    if (state == ST_ROUND_OVER || state == ST_GAME_OVER) {
        if (round_won) {
            attron(COLOR_PAIR(3) | A_BOLD);
            mvaddstr(ay + AREA_H / 2 - 2, ax + AREA_W / 2 - 6,
                     _("NICE SHOT!", "TIRO CERTO!"));
            attroff(COLOR_PAIR(3) | A_BOLD);
        } else {
            attron(COLOR_PAIR(4) | A_BOLD);
            int dogx = ax + AREA_W / 2 - 12;
            int dogy = ay + AREA_H - 4;
            mvaddstr(dogy, dogx, _("Ha ha! The dog laughs at you!",
                                    "Ha ha! O cachorro ri de voce!"));
            const char *dog[] = {
                "  ^___^",
                " ( o o )",
                "  \\   /",
                "   \\_/"
            };
            attron(COLOR_PAIR(2) | A_BOLD);
            for (int i = 0; i < 4; i++)
                mvaddstr(ay + AREA_H + 1 + i, ax + AREA_W / 2 + 8, dog[i]);
            attroff(COLOR_PAIR(2) | A_BOLD);
            attroff(COLOR_PAIR(4) | A_BOLD);
        }

        if (state == ST_GAME_OVER) {
            attron(COLOR_PAIR(5) | A_BOLD);
            snprintf(buf, 64, _("FINAL SCORE: %d", "PONTUACAO: %d"), score);
            mvaddstr(ay + AREA_H / 2 + 2, ax + AREA_W / 2 - 8, buf);
            mvaddstr(ay + AREA_H / 2 + 4, ax + AREA_W / 2 - 10,
                     _("Enter: play again  Q: quit",
                       "Enter: jogar novamente  Q: sair"));
            attroff(COLOR_PAIR(5) | A_BOLD);
        }
    }

    /* Instructions */
    attron(COLOR_PAIR(6) | A_DIM);
    mvaddstr(oy + 3 + AREA_H + 5, ox,
             _("Arrows: aim  Space: shoot  Q: quit",
               "Setas: mirar  Espaco: atirar  Q: sair"));
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
    init_theme_pair(5, COLOR_WHITE, COLOR_BLACK);
    init_theme_pair(6, COLOR_MAGENTA, COLOR_BLACK);
    apply_theme_bg();

    int play = 1;
    while (play) {
        new_game();

        while (1) {
            int ox = (COLS - AREA_W) / 2 < 1 ? 1 : (COLS - AREA_W) / 2;
            int oy = (LINES - 25) / 2 < 1 ? 1 : (LINES - 25) / 2;

            if (state == ST_ROUND_START) {
                draw(ox, oy);
                round_timer--;
                if (round_timer <= 0) state = ST_PLAYING;
                usleep(33000);
                continue;
            }

            if (state == ST_PLAYING) {
                update_ducks();
                if (all_done() || shots <= 0) {
                    round_won = (ducks_hit >= ducks_in_round);
                    if (!round_won) {
                        state = ST_GAME_OVER;
                    } else {
                        state = ST_ROUND_OVER;
                        round_timer = 60;
                    }
                }
            }

            if (state == ST_ROUND_OVER) {
                draw(ox, oy);
                round_timer--;
                if (round_timer <= 0) new_round();
                usleep(33000);
                continue;
            }

            draw(ox, oy);

            int ch = getch();
            if (ch == 'q') { play = 0; break; }

            if (state == ST_GAME_OVER) {
                if (ch == '\n' || ch == ' ') break;
                usleep(16000);
                continue;
            }

            if (state != ST_PLAYING) continue;

            /* Aim */
            if (ch == KEY_UP && cross_y > 1) cross_y--;
            else if (ch == KEY_DOWN && cross_y < AREA_H - 2) cross_y++;
            else if (ch == KEY_LEFT && cross_x > 1) cross_x--;
            else if (ch == KEY_RIGHT && cross_x < AREA_W - 3) cross_x++;
            else if (ch == 'w' && cross_y > 1) cross_y--;
            else if (ch == 's' && cross_y < AREA_H - 2) cross_y++;
            else if (ch == 'a' && cross_x > 1) cross_x--;
            else if (ch == 'd' && cross_x < AREA_W - 3) cross_x++;

            if (ch == ' ') shoot();

            usleep(33000);
        }
        if (!play) break;
    }

    endwin();
    return 0;
}
