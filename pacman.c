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

typedef struct { int y, x, dy, dx, fright; } Ghost;

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
        ghosts[i] = (Ghost){ gy[i], gx[i], -1, 0, 0 };
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
        for (int i = 0; i < MAX_GHOSTS; i++) ghosts[i].fright = FRIGHT_TIME;
    }
    if (dots_eaten >= dots_total) won = 1;
}

static void target_for_ghost(int i, int *ty, int *tx) {
    switch (i) {
        case 0:
            *ty = py; *tx = px;
            break;
        case 1:
            *ty = py + 4 * pdy;
            *tx = px + 4 * pdx;
            if (pdy == -1) *tx = px - 4;
            break;
        case 2: {
            int ahead_y = py + 2 * pdy;
            int ahead_x = px + 2 * pdx;
            *ty = 2 * ahead_y - ghosts[0].y;
            *tx = 2 * ahead_x - ghosts[0].x;
            break;
        }
        case 3: {
            int dist = abs(ghosts[i].y - py) + abs(ghosts[i].x - px);
            if (dist > 8) { *ty = py; *tx = px; }
            else { *ty = 20; *tx = 2; }
            break;
        }
    }
}

static void move_ghost_to_target(Ghost *g, int ty, int tx) {
    int best = 9999, bd = 0, flee = g->fright > 0;
    int dys[] = {-1, 1, 0, 0}, dxs[] = {0, 0, -1, 1};
    int order[] = {0, 1, 2, 3};
    for (int a = 3; a > 0; a--) {
        int b = rand() % (a + 1);
        int t = order[a]; order[a] = order[b]; order[b] = t;
    }
    for (int d = 0; d < 4; d++) {
        int od = order[d];
        int ny = g->y + dys[od], nx = g->x + dxs[od];
        if (nx < 0) nx = C - 1;
        if (nx >= C) nx = 0;
        if (ny < 0 || ny >= R || !is_walkable(ny, nx)) continue;
        if (!flee && dys[od] == -g->dy && dxs[od] == -g->dx) continue;
        int dist = abs(ny - ty) + abs(nx - tx);
        if (flee) { if (dist > best) { best = dist; bd = od; } }
        else { if (dist < best) { best = dist; bd = od; } }
    }
    if (best < 9999) { g->dy = dys[bd]; g->dx = dxs[bd]; }
}

static int scatter_mode = 0, scatter_timer = 0;
static int scatter_targets[4][2] = {{1, 25}, {1, 2}, {19, 25}, {19, 2}};

static void move_ghosts(void) {
    // Cycle between scatter and chase every 240 frames
    scatter_timer++;
    if (scatter_timer > 240) { scatter_timer = 0; scatter_mode = !scatter_mode; }

    for (int i = 0; i < MAX_GHOSTS; i++) {
        Ghost *g = &ghosts[i];
        int ty, tx;

        if (g->fright > 0) {
            target_for_ghost(i, &ty, &tx);
            move_ghost_to_target(g, ty, tx);
        } else if (scatter_mode) {
            ty = scatter_targets[i][0];
            tx = scatter_targets[i][1];
            move_ghost_to_target(g, ty, tx);
        } else {
            target_for_ghost(i, &ty, &tx);
            move_ghost_to_target(g, ty, tx);
        }

        g->y += g->dy; g->x += g->dx;
        if (g->x < 0) g->x = C - 1;
        if (g->x >= C) g->x = 0;
        // Safety: if ghost ended up in a wall, reset to ghost house
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
            if (frame % 5 == 0) move_ghosts();

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
                            for (int j = 0; j < MAX_GHOSTS; j++)
                                ghosts[j] = (Ghost){ 10, 14, 0, -1, 0 };
                            ghosts[0].x = 12; ghosts[1].x = 16;
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
