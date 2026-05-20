/******************************************************************************
 *                          CAMPO MINADO (Minesweeper)
 *                      Find all mines without exploding!
 ******************************************************************************/
#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include "config.h"

#define R 9
#define C 9
#define M 10

static int board[R][C], revealed[R][C], flagged[R][C], over, won;

static void init_game(void) {
    over = won = 0;
    for (int y = 0; y < R; y++)
        for (int x = 0; x < C; x++)
            board[y][x] = revealed[y][x] = flagged[y][x] = 0;

    int placed = 0;
    while (placed < M) {
        int y = rand() % R, x = rand() % C;
        if (board[y][x] != -1) { board[y][x] = -1; placed++; }
    }

    for (int y = 0; y < R; y++)
        for (int x = 0; x < C; x++)
            if (board[y][x] != -1) {
                int n = 0;
                for (int dy = -1; dy <= 1; dy++)
                    for (int dx = -1; dx <= 1; dx++)
                        if (y + dy >= 0 && y + dy < R && x + dx >= 0 && x + dx < C && board[y + dy][x + dx] == -1)
                            n++;
                board[y][x] = n;
            }
}

static void reveal(int y, int x) {
    if (y < 0 || y >= R || x < 0 || x >= C || revealed[y][x] || flagged[y][x]) return;
    revealed[y][x] = 1;
    if (board[y][x] == -1) { over = 1; return; }
    if (board[y][x] == 0)
        for (int dy = -1; dy <= 1; dy++)
            for (int dx = -1; dx <= 1; dx++)
                reveal(y + dy, x + dx);
}

static void check_win(void) {
    won = 1;
    for (int y = 0; y < R && won; y++)
        for (int x = 0; x < C && won; x++)
            if (!revealed[y][x] && board[y][x] != -1) won = 0;
}

int main(void) {
    srand(time(NULL));
    read_config();
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    start_color();

    init_theme_pair(1, COLOR_CYAN, COLOR_BLACK);
    init_theme_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_theme_pair(3, COLOR_BLUE, COLOR_BLACK);
    init_theme_pair(4, COLOR_YELLOW, COLOR_BLACK);
    init_theme_pair(5, COLOR_RED, COLOR_BLACK);
    init_theme_pair(6, COLOR_MAGENTA, COLOR_BLACK);
    init_theme_pair(7, COLOR_WHITE, COLOR_BLACK);
    init_theme_pair(8, COLOR_BLACK, COLOR_WHITE);
    apply_theme_bg();

    int play_again = 1;
    while (play_again) {
        init_game();

        int cy = 0, cx = 0, ch;
        int oy = (LINES - R * 2 - 2) / 2;
        int ox = (COLS - C * 4 - 2) / 2;

        while (!over && !won) {
            erase();

            // Title
            attron(COLOR_PAIR(1) | A_BOLD);
            mvaddstr(oy - 2, ox + 5, _("MINESWEEPER", "CAMPO MINADO"));
            attroff(COLOR_PAIR(1) | A_BOLD);

            // Border
            attron(COLOR_PAIR(8));
            for (int x = 0; x < C * 4 + 2; x++) {
                mvaddch(oy, ox + x, ' ');
                mvaddch(oy + R * 2 + 1, ox + x, ' ');
            }
            for (int y = 0; y < R * 2 + 2; y++) {
                mvaddch(oy + y, ox, ' ');
                mvaddch(oy + y, ox + C * 4 + 1, ' ');
            }
            attroff(COLOR_PAIR(8));

            for (int y = 0; y < R; y++) {
                for (int x = 0; x < C; x++) {
                    int yy = oy + y * 2 + 1;
                    int xx = ox + x * 4 + 1;

                    if (y == cy && x == cx) attron(A_REVERSE);

                    if (flagged[y][x]) {
                        attron(COLOR_PAIR(2) | A_BOLD);
                        mvaddstr(yy, xx, " F ");
                        attroff(COLOR_PAIR(2) | A_BOLD);
                    } else if (!revealed[y][x]) {
                        attron(COLOR_PAIR(7));
                        mvaddstr(yy, xx, " . ");
                        attroff(COLOR_PAIR(7));
                    } else if (board[y][x] == 0) {
                        mvaddstr(yy, xx, "   ");
                    } else if (board[y][x] == -1) {
                        attron(COLOR_PAIR(5) | A_BOLD);
                        mvaddstr(yy, xx, " X ");
                        attroff(COLOR_PAIR(5) | A_BOLD);
                    } else {
                        int n = board[y][x];
                        int c = (n <= 3) ? 3 : (n <= 5) ? 4 : 5;
                        attron(COLOR_PAIR(c) | A_BOLD);
                        char buf[4]; snprintf(buf, sizeof buf, " %d ", n);
                        mvaddstr(yy, xx, buf);
                        attroff(COLOR_PAIR(c) | A_BOLD);
                    }

                    if (y == cy && x == cx) attroff(A_REVERSE);
                }
            }

            // Status
            int remaining = M;
            for (int y = 0; y < R; y++)
                for (int x = 0; x < C; x++)
                    if (flagged[y][x]) remaining--;
            attron(COLOR_PAIR(7));
            char buf[64];
            snprintf(buf, sizeof buf, _("Mines left: %d", "Minas restantes: %d"), remaining);
            mvaddstr(oy + R * 2 + 3, ox, buf);
            mvaddstr(oy + R * 2 + 4, ox, _("Arrows: move | Enter: reveal | F: flag | Q: quit", "Setas: mover | Enter: revelar | F: flag | Q: sair"));
            attroff(COLOR_PAIR(7));

            refresh();

            ch = getch();
            if (ch == 'q') { play_again = 0; break; }
            if (ch == KEY_UP    && cy > 0) cy--;
            if (ch == KEY_DOWN  && cy < R - 1) cy++;
            if (ch == KEY_LEFT  && cx > 0) cx--;
            if (ch == KEY_RIGHT && cx < C - 1) cx++;
            if ((ch == 'f' || ch == 'F') && !revealed[cy][cx])
                flagged[cy][cx] = !flagged[cy][cx];
            if ((ch == '\n' || ch == ' ') && !flagged[cy][cx]) {
                reveal(cy, cx);
                if (!over) check_win();
            }
        }

        if (play_again) {
            erase();
            if (won) {
                attron(COLOR_PAIR(2) | A_BOLD);
                mvaddstr(oy + R, ox + 5, _("YOU WIN!", "VOCE VENCEU!"));
                attroff(COLOR_PAIR(2) | A_BOLD);
            } else {
                attron(COLOR_PAIR(5) | A_BOLD);
                mvaddstr(oy + R, ox + 5, _("BOOM! GAME OVER", "BOOM! GAME OVER"));
                attroff(COLOR_PAIR(5) | A_BOLD);
            }
            attron(COLOR_PAIR(7));
            mvaddstr(oy + R + 2, ox + 2, _("Enter: play again | Q: quit", "Enter: jogar novamente | Q: sair"));
            attroff(COLOR_PAIR(7));
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
