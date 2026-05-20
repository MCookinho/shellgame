/******************************************************************************
 *                              CENTIPEDE
 *         Shoot the creeping centipede before it reaches you!
 ******************************************************************************/
#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include "config.h"

#define W 26
#define H 20
#define MAX_SEG 60
#define MAX_CENTS 4
#define MAX_MUSH 25
#define MUSH_HITS 4
#define MAX_BULLETS 3

typedef struct { int x, y, active; } Bullet;

typedef struct {
    int segs[MAX_SEG][2]; // [i][0]=x, [i][1]=y
    int len;
    int dir; // 1 right, -1 left
} Cent;

static Cent cents[MAX_CENTS];
static int cent_count;
static int mush_x[MAX_MUSH], mush_y[MAX_MUSH], mush_hp[MAX_MUSH];
static int mush_count;
static Bullet bullets[MAX_BULLETS];
static int player_x;
static int score, lives, level, over, won;
static int frame;

static void init_game(void) {
    player_x = W / 2;
    score = 0; lives = 3; level = 1; over = 0; won = 0; frame = 0;

    // Init centipede
    cent_count = 1;
    int len = 8 + level * 2;
    if (len > MAX_SEG) len = MAX_SEG;
    cents[0].len = len;
    cents[0].dir = 1;
    int start_y = 1;
    for (int i = 0; i < len; i++) {
        cents[0].segs[i][0] = W / 2 - i;
        cents[0].segs[i][1] = start_y;
    }

    // Mushrooms
    mush_count = 8 + level * 4;
    if (mush_count > MAX_MUSH) mush_count = MAX_MUSH;
    for (int i = 0; i < mush_count; i++) {
        int tries = 0;
        do {
            mush_x[i] = 1 + rand() % (W - 2);
            mush_y[i] = 2 + rand() % (H - 6);
            tries++;
        } while (tries < 20 && (mush_x[i] < 3 || mush_y[i] < 2));
        mush_hp[i] = MUSH_HITS;
    }

    for (int i = 0; i < MAX_BULLETS; i++) bullets[i].active = 0;
}

static int has_mushroom(int x, int y) {
    for (int i = 0; i < mush_count; i++)
        if (mush_x[i] == x && mush_y[i] == y && mush_hp[i] > 0) return 1;
    return 0;
}

static void move_centipedes(void) {
    for (int c = 0; c < cent_count; c++) {
        if (cents[c].len <= 0) continue;
        int *hx = &cents[c].segs[0][0];
        int *hy = &cents[c].segs[0][1];
        int nx = *hx + cents[c].dir;
        int ny = *hy;

        // Check wall or mushroom collision
        if (nx <= 0 || nx >= W - 1 || has_mushroom(nx, ny)) {
            ny = *hy + 1;
            cents[c].dir *= -1;
            if (ny >= H - 1) {
                over = 1;
                return;
            }
        }

        // Move body: shift all segments forward
        for (int i = cents[c].len - 1; i > 0; i--) {
            cents[c].segs[i][0] = cents[c].segs[i - 1][0];
            cents[c].segs[i][1] = cents[c].segs[i - 1][1];
        }
        cents[c].segs[0][0] = nx;
        cents[c].segs[0][1] = ny;
    }
}

static void shoot(void) {
    for (int i = 0; i < MAX_BULLETS; i++)
        if (!bullets[i].active) {
            bullets[i] = (Bullet){ player_x, H - 2, 1 };
            break;
        }
}

static void move_bullets(void) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) continue;
        bullets[i].y--;
        if (bullets[i].y < 0) { bullets[i].active = 0; continue; }

        int bx = bullets[i].x, by = bullets[i].y;

        // Check mushroom hit
        int hit_mush = 0;
        for (int m = 0; m < mush_count; m++) {
            if (mush_x[m] == bx && mush_y[m] == by && mush_hp[m] > 0) {
                mush_hp[m]--;
                bullets[i].active = 0;
                hit_mush = 1;
                if (mush_hp[m] <= 0) score += 5;
                break;
            }
        }
        if (hit_mush) continue;

        // Check centipede hit
        for (int c = 0; c < cent_count; c++) {
            if (cents[c].len <= 0) continue;
            for (int s = 0; s < cents[c].len; s++) {
                if (cents[c].segs[s][0] == bx && cents[c].segs[s][1] == by) {
                    bullets[i].active = 0;
                    score += 10;

                    // Remove hit segment
                    int remaining = cents[c].len - s - 1;
                    if (remaining > 0) {
                        // Spawn new centipede going opposite direction
                        if (cent_count < MAX_CENTS) {
                            cents[cent_count].dir = -cents[c].dir;
                            cents[cent_count].len = remaining;
                            for (int k = 0; k < remaining; k++) {
                                cents[cent_count].segs[k][0] = cents[c].segs[s + 1 + k][0];
                                cents[cent_count].segs[k][1] = cents[c].segs[s + 1 + k][1];
                            }
                            cent_count++;
                        }
                    }
                    cents[c].len = s; // truncate at hit point

                    if (cents[c].len <= 0) {
                        // Remove this centipede by shifting
                        for (int k = c; k < cent_count - 1; k++)
                            cents[k] = cents[k + 1];
                        cent_count--;
                    }
                    goto bullet_done;
                }
            }
        }
        bullet_done:;
    }
}

static void draw_centipede(void) {
    for (int c = 0; c < cent_count; c++) {
        if (cents[c].len <= 0) continue;
        for (int i = cents[c].len - 1; i >= 0; i--) {
            int x = cents[c].segs[i][0], y = cents[c].segs[i][1];
            if (y < 0 || y >= H) continue;
            if (i == 0) {
                attron(COLOR_PAIR(3) | A_BOLD);
                mvaddch(y, x, 'O');
                attroff(COLOR_PAIR(3) | A_BOLD);
            } else {
                int color = (cents[c].len > 15) ? 5 : 3;
                attron(COLOR_PAIR(color));
                mvaddch(y, x, (i % 3 == 0) ? 'o' : '#');
                attroff(COLOR_PAIR(color));
            }
        }
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
    init_theme_pair(6, COLOR_MAGENTA, COLOR_BLACK);
    apply_theme_bg();

    int play_again = 1;
    while (play_again) {
        init_game();
        int ch;
        int oy = (LINES - H - 4) / 2;
        int ox = (COLS - W) / 2;

        while (1) {
            while (!over && !won) {
                ch = getch();
                if (ch == 'q') { over = 1; play_again = 0; break; }
                if ((ch == KEY_LEFT || ch == 'a') && player_x > 1) player_x--;
                if ((ch == KEY_RIGHT || ch == 'd') && player_x < W - 2) player_x++;
                if (ch == ' ' || ch == '\n') shoot();

                if (frame % (6 - level / 2) == 0)
                    move_centipedes();

                if (frame % 3 == 0)
                    move_bullets();

                // Check win (all centipedes dead)
                int alive = 0;
                for (int c = 0; c < cent_count; c++)
                    if (cents[c].len > 0) alive++;
                if (alive == 0) {
                    if (level < 3) { level++; won = 1; }
                    else won = 1;
                }

                erase();

                // Border
                attron(COLOR_PAIR(4));
                for (int x = 0; x < W; x++) {
                    mvaddch(oy + H, ox + x, '-');
                }
                for (int y = 0; y < H; y++) {
                    mvaddch(oy + y, ox, '|');
                    mvaddch(oy + y, ox + W - 1, '|');
                }
                attroff(COLOR_PAIR(4));

                // Mushrooms
                for (int i = 0; i < mush_count; i++) {
                    if (mush_hp[i] <= 0) continue;
                    int my = oy + mush_y[i], mx = ox + mush_x[i];
                    int broken = (mush_hp[i] < MUSH_HITS);
                    attron(COLOR_PAIR(6) | (broken ? A_DIM : A_BOLD));
                    mvaddch(my, mx, broken ? '$' : '%');
                    attroff(COLOR_PAIR(6) | (broken ? A_DIM : A_BOLD));
                }

                // Centipede
                draw_centipede();

                // Bullets
                attron(COLOR_PAIR(2) | A_BOLD);
                for (int i = 0; i < MAX_BULLETS; i++)
                    if (bullets[i].active)
                        mvaddch(oy + bullets[i].y, ox + bullets[i].x, '|');
                attroff(COLOR_PAIR(2) | A_BOLD);

                // Player
                attron(COLOR_PAIR(2) | A_BOLD);
                mvaddstr(oy + H - 1, ox + player_x - 1, "/A\\");
                attroff(COLOR_PAIR(2) | A_BOLD);

                // HUD
                attron(COLOR_PAIR(5));
                char buf[64];
                snprintf(buf, sizeof(buf), _("Score: %d  Lives: %d  Level: %d", "Pontos: %d  Vidas: %d  Nivel: %d"),
                         score, lives, level);
                mvaddstr(oy + H + 2, ox, buf);
                mvaddstr(oy + H + 3, ox,
                         _("<- ->: move | Space: shoot | Q: quit", "<- ->: mover | Espaco: atirar | Q: sair"));
                attroff(COLOR_PAIR(5));

                refresh();
                usleep(50000);
                frame++;
            }

            if (!play_again) break;

            if (won && level <= 3) {
                erase();
                char buf[64];
                attron(COLOR_PAIR(2) | A_BOLD);
                snprintf(buf, sizeof(buf), _("LEVEL %d CLEAR!", "NIVEL %d CONCLUIDO!"), level - 1);
                mvaddstr(oy + H / 2, ox + W / 2 - 8, buf);
                attroff(COLOR_PAIR(2) | A_BOLD);
                attron(COLOR_PAIR(5));
                mvaddstr(oy + H / 2 + 2, ox + W / 2 - 12,
                         _("Get ready for next level...", "Prepare-se..."));
                attroff(COLOR_PAIR(5));
                refresh();
                nodelay(stdscr, FALSE);
                getch();
                nodelay(stdscr, TRUE);
                // Re-init for next level
                cent_count = 1;
                int len = 8 + level * 2;
                if (len > MAX_SEG) len = MAX_SEG;
                cents[0].len = len;
                cents[0].dir = 1;
                for (int i = 0; i < len; i++) {
                    cents[0].segs[i][0] = W / 2 - i;
                    cents[0].segs[i][1] = 1;
                }
                mush_count = 8 + level * 4;
                if (mush_count > MAX_MUSH) mush_count = MAX_MUSH;
                for (int i = 0; i < mush_count; i++) {
                    mush_x[i] = 1 + rand() % (W - 2);
                    mush_y[i] = 2 + rand() % (H - 6);
                    mush_hp[i] = MUSH_HITS;
                }
                for (int i = 0; i < MAX_BULLETS; i++) bullets[i].active = 0;
                player_x = W / 2;
                won = 0;
                continue;
            }
            break;
        }

        if (play_again) {
            erase();
            char buf[64];
            if (won) {
                attron(COLOR_PAIR(2) | A_BOLD);
                mvaddstr(oy + H / 2, ox + W / 2 - 6, _("YOU WIN!", "VOCE VENCEU!"));
                attroff(COLOR_PAIR(2) | A_BOLD);
                attron(COLOR_PAIR(5));
                snprintf(buf, sizeof(buf), _("Final Score: %d", "Pontuacao Final: %d"), score);
                mvaddstr(oy + H / 2 + 2, ox + W / 2 - 8, buf);
            } else {
                attron(COLOR_PAIR(3) | A_BOLD);
                mvaddstr(oy + H / 2, ox + W / 2 - 6, _("GAME OVER", "FIM DE JOGO"));
                attroff(COLOR_PAIR(3) | A_BOLD);
                attron(COLOR_PAIR(5));
                snprintf(buf, sizeof(buf), _("Score: %d", "Pontuacao: %d"), score);
                mvaddstr(oy + H / 2 + 2, ox + W / 2 - 8, buf);
            }
            attron(COLOR_PAIR(5));
            mvaddstr(oy + H / 2 + 4, ox + W / 2 - 14,
                     _("Enter: play again | Q: quit", "Enter: jogar novamente | Q: sair"));
            attroff(COLOR_PAIR(5));
            refresh();
            nodelay(stdscr, FALSE);
            while (1) {
                ch = getch();
                if (ch == 'q') { play_again = 0; break; }
                if (ch == '\n') break;
            }
            nodelay(stdscr, TRUE);
        }
    }

    endwin();
    return 0;
}
