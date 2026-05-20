/******************************************************************************
 *                             DONKEY KONG
 *         Climb platforms, dodge barrels, save the princess!
 ******************************************************************************/
#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include "config.h"

#define ROWS 22
#define COLS 40
#define MAX_BARRELS 6

typedef struct { int y, x; } Point;
typedef struct { Point pos; int dir; } Barrel;

static char map[ROWS][COLS + 1];
static Point player, pauline, dk;
static Barrel barrels[MAX_BARRELS];
static int barrel_count, won, dead, frame, score;

static void init_map(void) {
    for (int y = 0; y < ROWS; y++)
        for (int x = 0; x < COLS; x++)
            map[y][x] = ' ';

    dk = (Point){ 0, COLS / 2 };
    pauline = (Point){ 0, COLS / 2 - 6 };
    player = (Point){ ROWS - 2, 5 };

    // Platforms
    for (int x = 0; x < COLS; x++) map[0][x] = '=';
    for (int x = 6; x < 34; x++) map[3][x] = '=';
    for (int x = 2; x < 30; x++) map[6][x] = '=';
    for (int x = 10; x < 38; x++) map[9][x] = '=';
    for (int x = 4; x < 26; x++) map[12][x] = '=';
    for (int x = 14; x < 38; x++) map[15][x] = '=';
    for (int x = 2; x < 30; x++) map[18][x] = '=';

    // Ladders
    int ladders[][4] = {
        {2,14,2,15}, {3,14,3,15},
        {5,8,5,9}, {6,8,6,9},
        {8,28,8,29}, {9,28,9,29},
        {11,16,11,17}, {12,16,12,17},
        {14,34,14,35}, {15,34,15,35},
        {17,6,17,7}, {18,6,18,7},
    };
    for (int i = 0; i < 12; i++) {
        map[ladders[i][0]][ladders[i][1]] = '#';
        map[ladders[i][2]][ladders[i][3]] = '#';
    }

    barrel_count = 0;
    for (int i = 0; i < MAX_BARRELS; i++)
        barrels[i] = (Barrel){ .dir = 1 };
}

static int is_platform(int y, int x) {
    return (y >= 0 && y < ROWS && x >= 0 && x < COLS && map[y][x] == '=');
}

static int is_ladder(int y, int x) {
    return (y >= 0 && y < ROWS && x >= 0 && x < COLS && map[y][x] == '#');
}

static int is_walkable(int y, int x) {
    return (y >= 0 && y < ROWS && x >= 0 && x < COLS &&
            (map[y][x] == ' ' || map[y][x] == 'K'));
}

static void spawn_barrel(void) {
    if (barrel_count >= MAX_BARRELS) return;
    for (int i = 0; i < MAX_BARRELS; i++) {
        if (barrels[i].pos.y == 0 && barrels[i].pos.x == 0) {
            int sx = dk.x - 1;
            while (sx > 0 && map[2][sx - 1] == '=') sx--;
            sx++;
            barrels[i] = (Barrel){ {2, sx}, 1 };
            barrel_count++;
            break;
        }
    }
}

static void move_barrels(void) {
    for (int i = 0; i < MAX_BARRELS; i++) {
        if (barrels[i].pos.y == 0 && barrels[i].pos.x == 0) continue;
        Barrel *b = &barrels[i];
        int ny = b->pos.y, nx = b->pos.x + b->dir;

        if (nx < 1 || nx >= COLS - 1 || (map[ny][nx] != ' ' && map[ny][nx] != 'M')) {
            b->dir *= -1;
            nx = b->pos.x + b->dir;
        }

        if (is_platform(ny + 1, nx)) {
            b->pos.x = nx;
        } else if (map[ny + 1][nx] == ' ') {
            b->pos.y++;
            b->pos.x = nx;
            while (b->pos.y < ROWS - 1 && map[b->pos.y + 1][b->pos.x] == ' ')
                b->pos.y++;
            if (b->pos.y >= ROWS - 1) { b->pos = (Point){ 0, 0 }; barrel_count--; }
        } else {
            b->pos.x = nx;
        }

        if (b->pos.y == player.y && b->pos.x == player.x) dead = 1;
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

    init_theme_pair(1, COLOR_RED, COLOR_BLACK);
    init_theme_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_theme_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_theme_pair(4, COLOR_BLUE, COLOR_BLACK);
    init_theme_pair(5, COLOR_MAGENTA, COLOR_BLACK);
    init_theme_pair(6, COLOR_CYAN, COLOR_BLACK);
    init_theme_pair(7, COLOR_WHITE, COLOR_BLACK);
    apply_theme_bg();

    int play_again = 1;
    while (play_again) {
        score = won = dead = frame = 0;
        init_map();

        int ch;
        int jump = 0, jump_timer = 0;
        int oy = (LINES - ROWS) / 2;
        int ox = (COLS - COLS) / 2;

        while (!won && !dead) {
            ch = getch();
            if (ch == 'q') { dead = 1; play_again = 0; break; }

            int ny = player.y, nx = player.x;

            if ((ch == KEY_LEFT || ch == 'a') && nx > 0 && is_walkable(ny, nx - 1) && !is_platform(ny, nx - 1)) nx--;
            if ((ch == KEY_RIGHT || ch == 'd') && nx < COLS - 1 && is_walkable(ny, nx + 1) && !is_platform(ny, nx + 1)) nx++;
            if ((ch == KEY_UP || ch == 'w') && is_ladder(ny - 1, nx)) ny--;
            if ((ch == KEY_DOWN || ch == 's') && is_ladder(ny + 1, nx)) ny++;

            if ((ch == ' ' || ch == KEY_UP) && !jump) { jump = 1; jump_timer = 4; }

            if (jump && jump_timer > 0) {
                if (is_ladder(ny - 1, nx) || is_walkable(ny - 1, nx)) ny--;
                jump_timer--;
                if (jump_timer == 0) jump = 0;
            } else if (jump) jump = 0;

            if (!jump && !is_ladder(ny, nx) && ny < ROWS - 1 && is_walkable(ny + 1, nx) && !is_platform(ny + 1, nx)) {
                int fall = 0;
                while (ny < ROWS - 1 && is_walkable(ny + 1, nx) && !is_platform(ny + 1, nx)) {
                    ny++; fall++;
                }
                if (fall > 4) dead = 1;
            }

            if (ny < 0 || ny >= ROWS || nx < 0 || nx >= COLS) ny = player.y;
            if (map[ny][nx] == '=' || map[ny][nx] == 'K') { ny = player.y; nx = player.x; }
            player = (Point){ ny, nx };

            if (player.y == pauline.y && abs(player.x - pauline.x) < 4) won = 1;

            if (frame % 30 == 0 && barrel_count < MAX_BARRELS) spawn_barrel();
            if (frame % 5 == 0) move_barrels();

            erase();

            // Draw map
            for (int y = 0; y < ROWS; y++) {
                for (int x = 0; x < COLS; x++) {
                    char ch = map[y][x];
                    if (ch == '=') { attron(COLOR_PAIR(7) | A_REVERSE); mvaddch(oy + y, ox + x, ' '); attroff(COLOR_PAIR(7) | A_REVERSE); }
                    else if (ch == '#') { attron(COLOR_PAIR(4) | A_BOLD); mvaddch(oy + y, ox + x, 'H'); attroff(COLOR_PAIR(4) | A_BOLD); }
                    else mvaddch(oy + y, ox + x, ' ');
                }
            }

            // Characters
            attron(COLOR_PAIR(2) | A_BOLD); mvaddch(oy + player.y, ox + player.x, 'M'); attroff(COLOR_PAIR(2) | A_BOLD);
            attron(COLOR_PAIR(5) | A_BOLD); mvaddch(oy + pauline.y, ox + pauline.x, 'L'); attroff(COLOR_PAIR(5) | A_BOLD);
            attron(COLOR_PAIR(1) | A_BOLD); mvaddch(oy + dk.y, ox + dk.x, 'D'); attroff(COLOR_PAIR(1) | A_BOLD);

            // Barrels
            for (int i = 0; i < MAX_BARRELS; i++)
                if (barrels[i].pos.y != 0 || barrels[i].pos.x != 0) {
                    attron(COLOR_PAIR(3) | A_BOLD); mvaddch(oy + barrels[i].pos.y, ox + barrels[i].pos.x, 'B'); attroff(COLOR_PAIR(3) | A_BOLD);
                }

            // UI
            attron(COLOR_PAIR(6));
            char buf[64];
            snprintf(buf, sizeof buf, "Score: %d", score);
            mvaddstr(oy + ROWS + 1, ox, buf);
            mvaddstr(oy + ROWS + 2, ox, _("WASD/Arrows: move | Space: jump | Q: quit", "WASD/Setas: mover | Espaco: pular | Q: sair"));
            attroff(COLOR_PAIR(6));

            refresh();
            usleep(70000);
            frame++;
        }

        if (play_again) {
            erase();
            attron(A_BOLD);
            if (won) { attron(COLOR_PAIR(2)); mvaddstr(oy + ROWS / 2, ox + 10, _("YOU WIN!", "VOCE VENCEU!")); attroff(COLOR_PAIR(2)); }
            else { attron(COLOR_PAIR(1)); mvaddstr(oy + ROWS / 2, ox + 10, "GAME OVER!"); attroff(COLOR_PAIR(1)); }
            attroff(A_BOLD);
            attron(COLOR_PAIR(6));
            char buf[64];
            snprintf(buf, sizeof buf, "Score: %d", score);
            mvaddstr(oy + ROWS / 2 + 2, ox + 12, buf);
            mvaddstr(oy + ROWS / 2 + 4, ox + 6, _("Enter: play again | Q: quit", "Enter: jogar novamente | Q: sair"));
            attroff(COLOR_PAIR(6));
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
