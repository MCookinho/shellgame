/******************************************************************************
 *                                2048
 *               Slide and merge tiles to reach 2048!
 ******************************************************************************/
#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include "config.h"

#define S 4

static int board[S][S];
static int score, won, over;

static void init_board(void) {
    score = won = over = 0;
    for (int y = 0; y < S; y++)
        for (int x = 0; x < S; x++)
            board[y][x] = 0;
}

static void add_tile(void) {
    int empty[S * S][2], n = 0;
    for (int y = 0; y < S; y++)
        for (int x = 0; x < S; x++)
            if (board[y][x] == 0) { empty[n][0] = y; empty[n][1] = x; n++; }
    if (n == 0) return;
    int idx = rand() % n;
    board[empty[idx][0]][empty[idx][1]] = (rand() % 10 < 9) ? 2 : 4;
}

static void rotate(int dir) {
    int tmp[S][S];
    for (int y = 0; y < S; y++)
        for (int x = 0; x < S; x++)
            tmp[y][x] = board[y][x];
    for (int y = 0; y < S; y++)
        for (int x = 0; x < S; x++) {
            if (dir == 0) board[y][x] = tmp[S - 1 - x][y];
            if (dir == 1) board[y][x] = tmp[x][S - 1 - y];
            if (dir == 2) board[y][x] = tmp[S - 1 - y][S - 1 - x];
        }
}

static int slide(void) {
    int moved = 0;
    for (int y = 0; y < S; y++) {
        // Compact
        int wp = 0;
        for (int x = 0; x < S; x++)
            if (board[y][x] != 0) {
                if (wp != x) { board[y][wp] = board[y][x]; board[y][x] = 0; moved = 1; }
                wp++;
            }
        // Merge
        for (int x = 0; x < S - 1; x++) {
            if (board[y][x] != 0 && board[y][x] == board[y][x + 1]) {
                board[y][x] *= 2;
                score += board[y][x];
                if (board[y][x] == 2048) won = 1;
                board[y][x + 1] = 0;
                moved = 1;
                // Compact again
                for (int k = x + 1; k < S - 1; k++)
                    board[y][k] = board[y][k + 1];
                board[y][S - 1] = 0;
            }
        }
    }
    return moved;
}

static int slide_move(int dy, int dx) {
    // Rotate so we always slide left
    if (dy == -1) rotate(0);  // up -> rotate left
    else if (dy == 1) rotate(1); // down -> rotate right
    else if (dx == 1) rotate(2); // right -> rotate 180

    int moved = slide();

    if (dy == -1) rotate(1); // undo
    else if (dy == 1) rotate(0);
    else if (dx == 1) rotate(2);

    return moved;
}

static int can_move(void) {
    for (int y = 0; y < S; y++)
        for (int x = 0; x < S; x++) {
            if (board[y][x] == 0) return 1;
            if (x < S - 1 && board[y][x] == board[y][x + 1]) return 1;
            if (y < S - 1 && board[y][x] == board[y + 1][x]) return 1;
        }
    return 0;
}

static int color_for(int val) {
    switch (val) {
        case 0: return 0;
        case 2: return 1;
        case 4: return 2;
        case 8: return 3;
        case 16: return 4;
        case 32: return 5;
        case 64: return 6;
        case 128: return 7;
        case 256: return 8;
        case 512: return 9;
        case 1024: return 10;
        case 2048: return 11;
        default: return 12;
    }
}

static void draw_tile(int y, int x, int val) {
    char buf[8];
    if (val == 0) { mvprintw(y, x, "    "); return; }
    snprintf(buf, sizeof buf, "%d", val);
    int len = strlen(buf);
    int pad = (4 - len) / 2;
    for (int i = 0; i < pad; i++) mvaddch(y, x + i, ' ');
    mvaddstr(y, x + pad, buf);
    for (int i = pad + len; i < 4; i++) mvaddch(y, x + i, ' ');
}

int main(void) {
    srand(time(NULL));
    read_config();
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    nodelay(stdscr, FALSE);
    start_color();

    init_theme_pair(1, COLOR_BLACK, COLOR_WHITE);
    init_theme_pair(2, COLOR_BLACK, COLOR_YELLOW);
    init_theme_pair(3, COLOR_WHITE, COLOR_RED);
    init_theme_pair(4, COLOR_WHITE, COLOR_MAGENTA);
    init_theme_pair(5, COLOR_WHITE, COLOR_BLUE);
    init_theme_pair(6, COLOR_WHITE, COLOR_CYAN);
    init_theme_pair(7, COLOR_WHITE, COLOR_GREEN);
    init_theme_pair(8, COLOR_BLACK, COLOR_YELLOW);
    init_theme_pair(9, COLOR_WHITE, COLOR_RED);
    init_theme_pair(10, COLOR_WHITE, COLOR_MAGENTA);
    init_theme_pair(11, COLOR_YELLOW, COLOR_RED);
    init_theme_pair(12, COLOR_WHITE, COLOR_BLACK);
    apply_theme_bg();

    int play_again = 1;
    while (play_again) {
        init_board();
        add_tile();
        add_tile();

        int ch;
        int oy = (LINES - S * 3) / 2;
        int ox = (COLS - S * 5) / 2;

        while (!won && !over) {
            erase();

            // Title
            attron(COLOR_PAIR(3) | A_BOLD);
            mvaddstr(oy - 2, ox + 2, "2048");
            attroff(COLOR_PAIR(3) | A_BOLD);

            // Grid
            for (int y = 0; y < S; y++) {
                for (int x = 0; x < S; x++) {
                    int val = board[y][x];
                    int c = color_for(val);
                    int yy = oy + y * 3;
                    int xx = ox + x * 5;

                    // Top border
                    attron(COLOR_PAIR(12));
                    for (int k = 0; k < 6; k++) mvaddch(yy, xx + k, ' ');
                    attroff(COLOR_PAIR(12));

                    if (c > 0) {
                        attron(COLOR_PAIR(c) | A_BOLD);
                        for (int k = 0; k < 5; k++) mvaddch(yy + 1, xx + k, ' ');
                        draw_tile(yy + 1, xx, val);
                        for (int k = 0; k < 5; k++) mvaddch(yy + 2, xx + k, ' ');
                        attroff(COLOR_PAIR(c) | A_BOLD);
                    } else {
                        for (int k = 0; k < 5; k++) mvaddch(yy + 1, xx + k, ' ');
                        for (int k = 0; k < 5; k++) mvaddch(yy + 2, xx + k, ' ');
                    }
                }
            }

            // Bottom border of last row
            {
                int yy = oy + S * 3;
                attron(COLOR_PAIR(12));
                for (int x = 0; x < S; x++)
                    for (int k = 0; k < 6; k++)
                        mvaddch(yy, ox + x * 5 + k, ' ');
                attroff(COLOR_PAIR(12));
            }

            // Info
            attron(COLOR_PAIR(1));
            char buf[64];
            snprintf(buf, sizeof buf, "Score: %d", score);
            mvaddstr(oy + S * 3 + 2, ox, buf);
            mvaddstr(oy + S * 3 + 3, ox, _("Arrows/WASD: move | Q: quit", "Setas/WASD: mover | Q: sair"));
            attroff(COLOR_PAIR(1));

            refresh();

            ch = getch();
            if (ch == 'q') { play_again = 0; break; }

            int moved = 0;
            if (ch == KEY_UP    || ch == 'w') moved = slide_move(-1, 0);
            if (ch == KEY_DOWN  || ch == 's') moved = slide_move(1, 0);
            if (ch == KEY_LEFT  || ch == 'a') moved = slide_move(0, -1);
            if (ch == KEY_RIGHT || ch == 'd') moved = slide_move(0, 1);

            if (moved) add_tile();
            if (!can_move()) over = 1;
        }

        if (play_again) {
            erase();
            if (won) {
                attron(COLOR_PAIR(11) | A_BOLD);
                mvaddstr(oy + S, ox + 2, _("CONGRATULATIONS! 2048!", "PARABENS! 2048!"));
                attroff(COLOR_PAIR(11) | A_BOLD);
            } else {
                attron(COLOR_PAIR(3) | A_BOLD);
                mvaddstr(oy + S, ox, _("GAME OVER - No moves left!", "GAME OVER - Sem movimentos!"));
                attroff(COLOR_PAIR(3) | A_BOLD);
            }
            attron(COLOR_PAIR(1));
            char buf[64];
            snprintf(buf, sizeof buf, _("Final score: %d", "Pontuacao final: %d"), score);
            mvaddstr(oy + S + 2, ox + 2, buf);
            mvaddstr(oy + S + 4, ox, _("Enter: play again | Q: quit", "Enter: jogar novamente | Q: sair"));
            attroff(COLOR_PAIR(1));
            refresh();
            while (1) {
                ch = getch();
                if (ch == 'q') { play_again = 0; break; }
                if (ch == '\n') { play_again = 1; break; }
            }
        }
    }

    endwin();
    return 0;
}
