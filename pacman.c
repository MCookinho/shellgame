/******************************************************************************
 *                               PAC-MAN
 *            Eat dots, avoid ghosts, clear the maze!
 ******************************************************************************/
#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include "config.h"

#define R 22
#define C 28
#define MAX_GHOSTS 4
#define FRIGHT_TIME 240

static int map[R][C];
static int dots_total, dots_eaten;

typedef struct { int y, x, dy, dx, fright, in_house; } Ghost;

static Ghost ghosts[MAX_GHOSTS];
static int py, px, pdy, pdx, dead, won, frame, lives, scared;

static int init_map_data[R][C] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,2,2,2,2,2,2,2,2,2,2,2,2,1,1,2,2,2,2,2,2,2,2,2,2,2,2,1},
    {1,2,1,1,1,1,2,1,1,1,1,1,2,1,1,2,1,1,1,1,1,2,1,1,1,1,2,1},
    {1,3,1,1,1,1,2,1,1,1,1,1,2,1,1,2,1,1,1,1,1,2,1,1,1,1,3,1},
    {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
    {1,2,1,1,1,1,2,1,1,2,1,1,1,1,1,1,1,1,2,1,1,2,1,1,1,1,2,1},
    {1,2,2,2,2,2,2,1,1,2,2,2,2,1,1,2,2,2,2,1,1,2,2,2,2,2,2,1},
    {1,1,1,1,1,1,2,1,1,1,1,1,0,1,1,0,1,1,1,1,1,2,1,1,1,1,1,1},
    {0,0,0,0,0,1,2,1,1,1,1,1,0,1,1,0,1,1,1,1,1,2,1,0,0,0,0,0},
    {1,1,1,1,1,1,2,1,1,0,0,0,0,0,0,0,0,0,0,1,1,2,1,1,1,1,1,1},
    {0,0,0,0,0,0,2,1,1,0,1,1,1,5,5,1,1,1,0,1,1,2,0,0,0,0,0,0},
    {1,1,1,1,1,1,2,1,1,0,1,0,0,0,0,0,0,1,0,1,1,2,1,1,1,1,1,1},
    {0,0,0,0,0,0,2,1,1,0,1,0,0,0,0,0,0,1,0,1,1,2,0,0,0,0,0,0},
    {1,1,1,1,1,1,2,1,1,0,0,0,0,0,0,0,0,0,0,1,1,2,1,1,1,1,1,1},
    {0,0,0,0,0,1,2,1,1,0,1,1,1,1,1,1,1,1,0,1,1,2,1,0,0,0,0,0},
    {0,0,0,0,0,1,2,1,1,0,1,1,1,1,1,1,1,1,0,1,1,2,1,0,0,0,0,0},
    {1,1,1,1,1,1,2,1,1,0,0,0,0,0,0,0,0,0,0,1,1,2,1,1,1,1,1,1},
    {1,2,2,2,2,2,2,2,2,2,2,2,2,1,1,2,2,2,2,2,2,2,2,2,2,2,2,1},
    {1,2,1,1,1,1,2,1,1,1,1,1,2,1,1,2,1,1,1,1,1,2,1,1,1,1,2,1},
    {1,2,2,2,1,1,2,2,2,2,2,2,2,0,0,2,2,2,2,2,2,2,1,1,2,2,2,1},
    {1,1,1,2,1,1,2,1,1,2,1,1,1,1,1,1,1,1,2,1,1,2,1,1,2,1,1,1},
    {1,2,2,2,2,2,2,2,2,2,2,2,2,1,1,2,2,2,2,2,2,2,2,2,2,2,2,1},
};

static void init_game(void) {
    dots_total = dots_eaten = 0;
    dead = won = 0;
    frame = 0;
    lives = 3;
    scared = 0;
    py = 16; px = 14; pdy = 0; pdx = -1;

    for (int y = 0; y < R; y++)
        for (int x = 0; x < C; x++) {
            map[y][x] = init_map_data[y][x];
            if (map[y][x] == 2 || map[y][x] == 3) dots_total++;
        }

    int gx[] = {14, 13, 14, 13};
    int gy[] = {10, 10, 11, 11};
    for (int i = 0; i < MAX_GHOSTS; i++)
        ghosts[i] = (Ghost){ gy[i], gx[i], 0, 0, 0, i > 0 };
}

static int is_walkable(int y, int x) {
    if (y < 0 || y >= R) return 0;
    if (x < 0) x = C - 1;
    if (x >= C) x = 0;
    return map[y][x] != 1;
}

static void move_pacman(void) {
    int ny = py + pdy, nx = px + pdx;
    if (nx < 0) nx = C - 1;
    if (nx >= C) nx = 0;
    if (is_walkable(ny, nx)) { py = ny; px = nx; }

    if (map[py][px] == 2) { map[py][px] = 0; dots_eaten++; }
    if (map[py][px] == 3) {
        map[py][px] = 0; dots_eaten++;
        scared = FRIGHT_TIME;
        for (int i = 0; i < MAX_GHOSTS; i++) {
            ghosts[i].fright = FRIGHT_TIME;
            ghosts[i].dy = -ghosts[i].dy;
            ghosts[i].dx = -ghosts[i].dx;
        }
    }
    if (dots_eaten >= dots_total) won = 1;
}

static void target_for_ghost(int i, int *ty, int *tx) {
    switch (i) {
        case 0: // Blinky: targets Pac-Man directly
            *ty = py; *tx = px;
            break;
        case 1: // Pinky: targets 4 tiles ahead of Pac-Man
            *ty = py + 4 * pdy;
            *tx = px + 4 * pdx;
            if (pdy == -1) *tx = px - 4; // original bug: facing up adds -4 to x
            break;
        case 2: { // Inky: vector from Blinky to 2 ahead, doubled
            int ahead_y = py + 2 * pdy;
            int ahead_x = px + 2 * pdx;
            *ty = 2 * ahead_y - ghosts[0].y;
            *tx = 2 * ahead_x - ghosts[0].x;
            break;
        }
        case 3: { // Clyde: chase when far, scatter when close
            int dy = py - ghosts[i].y, dx = px - ghosts[i].x;
            int dist = dy * dy + dx * dx;
            if (dist > 64) { *ty = py; *tx = px; }
            else { *ty = 20; *tx = 2; }
            break;
        }
    }
}

static void move_ghost_to_target(Ghost *g, int ty, int tx) {
    int dys[] = {-1, 1, 0, 0}, dxs[] = {0, 0, -1, 1};
    int porder[] = {0, 3, 1, 2}; // UP, RIGHT, DOWN, LEFT (tiebreak priority)
    int best = 9999, best_dir = -1;

    if (g->fright > 0) {
        // Frightened: random direction at each opportunity
        int valid[4], vc = 0;
        for (int d = 0; d < 4; d++) {
            if (dys[d] == -g->dy && dxs[d] == -g->dx) continue;
            int ny = g->y + dys[d], nx = g->x + dxs[d];
            if (nx < 0) nx = C - 1;
            if (nx >= C) nx = 0;
            if (ny < 0 || ny >= R || !is_walkable(ny, nx)) continue;
            valid[vc++] = d;
        }
        if (vc > 0) {
            int d = valid[rand() % vc];
            g->dy = dys[d]; g->dx = dxs[d];
        }
        return;
    }

    for (int p = 0; p < 4; p++) {
        int d = porder[p];
        int ny = g->y + dys[d], nx = g->x + dxs[d];
        if (nx < 0) nx = C - 1;
        if (nx >= C) nx = 0;
        if (ny < 0 || ny >= R || !is_walkable(ny, nx)) continue;
        if (dys[d] == -g->dy && dxs[d] == -g->dx) continue;
        int dist = (ny - ty) * (ny - ty) + (nx - tx) * (nx - tx);
        if (dist < best) { best = dist; best_dir = d; }
    }
    if (best_dir >= 0) { g->dy = dys[best_dir]; g->dx = dxs[best_dir]; }
}

static int scatter_mode = 0, phase_idx = 0, phase_timer = 0;
/* Scatter/chase phase durations (frames at 60fps) */
static int phase_durations[][2] = {{420, 1200}, {420, 1200}, {300, 1200}, {1, 99999}};
static int scatter_targets[4][2] = {{1, 26}, {1, 1}, {20, 27}, {20, 1}};
static int release_counts[] = {0, 30, 60, 100};

static void move_ghosts(void) {
    phase_timer++;
    if (phase_timer > phase_durations[phase_idx][scatter_mode]) {
        phase_timer = 0;
        scatter_mode = !scatter_mode;
        if (scatter_mode == 0 && phase_idx < 3) phase_idx++;
    }

    for (int i = 0; i < MAX_GHOSTS; i++) {
        Ghost *g = &ghosts[i];

        if (g->in_house) {
            if (dots_eaten >= release_counts[i]) {
                g->in_house = 0;
                g->dy = -1; g->dx = 0;
                if (g->y > 9) { g->y--; }
            } else {
                g->y += (frame / 30) % 2 == 0 ? 1 : -1;
                if (g->y < 10 || g->y > 11) g->dy = -g->dy;
                continue;
            }
        }

        if (g->fright > 0) {
            move_ghost_to_target(g, 0, 0);
        } else if (scatter_mode) {
            move_ghost_to_target(g, scatter_targets[i][0], scatter_targets[i][1]);
        } else {
            int ty, tx;
            target_for_ghost(i, &ty, &tx);
            move_ghost_to_target(g, ty, tx);
        }

        g->y += g->dy; g->x += g->dx;
        if (g->x < 0) g->x = C - 1;
        if (g->x >= C) g->x = 0;
        if (g->y < 0 || g->y >= R || map[g->y][g->x] == 1) {
            g->y = 10; g->x = 14; g->dy = -1; g->dx = 0;
        }
        if (g->fright > 0) g->fright--;
    }
    if (scared > 0) scared--;
}

int main(void) {
    read_config();
    srand(time(NULL));
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    start_color();

    init_theme_pair(1, COLOR_YELLOW, COLOR_BLACK);
    init_theme_pair(2, COLOR_RED, COLOR_BLACK);
    init_theme_pair(3, COLOR_MAGENTA, COLOR_BLACK);
    init_theme_pair(4, COLOR_CYAN, COLOR_BLACK);
    init_theme_pair(5, COLOR_BLUE, COLOR_BLACK);
    init_theme_pair(6, COLOR_GREEN, COLOR_BLACK);
    init_theme_pair(7, COLOR_WHITE, COLOR_BLACK);
    init_theme_pair(8, COLOR_CYAN, COLOR_BLACK);
    apply_theme_bg();

    int play_again = 1;
    while (play_again) {
        init_game();

        int ch;
        int oy = (LINES - R) / 2;
        int ox = (COLS - C * 2) / 2;

        while (lives > 0 && !won) {
            ch = getch();
            if (ch == 'q') { lives = 0; play_again = 0; break; }

            if (ch == KEY_UP    || ch == 'w') { pdy = -1; pdx = 0; }
            if (ch == KEY_DOWN  || ch == 's') { pdy =  1; pdx = 0; }
            if (ch == KEY_LEFT  || ch == 'a') { pdy =  0; pdx = -1; }
            if (ch == KEY_RIGHT || ch == 'd') { pdy =  0; pdx =  1; }

            if (frame % 3 == 0) move_pacman();
            if (frame % 4 == 0) move_ghosts();

            for (int i = 0; i < MAX_GHOSTS; i++)
                if (ghosts[i].y == py && ghosts[i].x == px) {
                    if (ghosts[i].fright > 0) {
                        ghosts[i].y = 10; ghosts[i].x = 14;
                        ghosts[i].dy = 0; ghosts[i].dx = 0;
                        ghosts[i].fright = 0;
                    } else {
                        lives--;
                        if (lives > 0) {
                            py = 16; px = 14; pdy = 0; pdx = -1;
                            for (int j = 0; j < MAX_GHOSTS; j++) {
                                int gx[] = {14, 13, 14, 13};
                                int gy[] = {10, 10, 11, 11};
                                ghosts[j] = (Ghost){ gy[j], gx[j], 0, 0, 0, j > 0 };
                            }
                            usleep(800000);
                        } else dead = 1;
                    }
                }

            erase();

            // Draw maze
            for (int y = 0; y < R; y++) {
                for (int x = 0; x < C; x++) {
                    int ch = map[y][x];
                    int yy = oy + y;
                    int xx = ox + x * 2;
                    if (ch == 1) { attron(COLOR_PAIR(8) | A_REVERSE); mvaddch(yy, xx, ' '); mvaddch(yy, xx + 1, ' '); attroff(COLOR_PAIR(8) | A_REVERSE); }
                    else if (ch == 2) { attron(COLOR_PAIR(7)); mvaddch(yy, xx, '.'); mvaddch(yy, xx + 1, ' '); attroff(COLOR_PAIR(7)); }
                    else if (ch == 3) { attron(COLOR_PAIR(7) | A_BOLD); mvaddch(yy, xx, 'O'); mvaddch(yy, xx + 1, ' '); attroff(COLOR_PAIR(7) | A_BOLD); }
                    else mvaddch(yy, xx + 1, ' ');
                }
            }

            // Draw ghosts
            for (int i = 0; i < MAX_GHOSTS; i++) {
                Ghost g = ghosts[i];
                int yy = oy + g.y;
                int xx = ox + g.x * 2;
                if (g.fright > 0) { attron(COLOR_PAIR(6) | A_BOLD); mvaddch(yy, xx, '?'); attroff(COLOR_PAIR(6) | A_BOLD); }
                else { attron(COLOR_PAIR(i + 2) | A_BOLD); mvaddch(yy, xx, 'G'); attroff(COLOR_PAIR(i + 2) | A_BOLD); }
            }

            // Draw Pac-Man
            attron(COLOR_PAIR(1) | A_BOLD);
            mvaddch(oy + py, ox + px * 2, 'C');
            attroff(COLOR_PAIR(1) | A_BOLD);

            // UI
            attron(COLOR_PAIR(7));
            char buf[64];
            snprintf(buf, sizeof buf, _("Score: %d  Lives: %d  Dots: %d/%d", "Pontuacao: %d  Vidas: %d  Dots: %d/%d"),
                     dots_eaten * 10, lives, dots_eaten, dots_total);
            mvaddstr(oy + R + 1, ox, buf);
            mvaddstr(oy + R + 2, ox, _("WASD/Arrows: move | Q: quit", "WASD/Setas: mover | Q: sair"));
            attroff(COLOR_PAIR(7));

            refresh();
            usleep(60000);
            frame++;
        }

        if (play_again) {
            erase();
            attron(A_BOLD);
            if (won) { attron(COLOR_PAIR(6)); mvaddstr(oy + R / 2, ox + 8, _("YOU WIN!", "VOCE VENCEU!")); attroff(COLOR_PAIR(6)); }
            else { attron(COLOR_PAIR(2)); mvaddstr(oy + R / 2, ox + 8, "GAME OVER!"); attroff(COLOR_PAIR(2)); }
            attroff(A_BOLD);
            attron(COLOR_PAIR(7));
            char buf[64];
            snprintf(buf, sizeof buf, _("Final score: %d", "Pontuacao final: %d"), dots_eaten * 10);
            mvaddstr(oy + R / 2 + 2, ox + 8, buf);
            mvaddstr(oy + R / 2 + 4, ox + 4, _("Enter: play again | Q: quit", "Enter: jogar novamente | Q: sair"));
            attroff(COLOR_PAIR(7));
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
