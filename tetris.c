/******************************************************************************
 *                                TETRIS
 *            Classic falling blocks with color, preview and levels
 ******************************************************************************/
#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include "config.h"

#define W 10
#define H 20
#define S 4

static int board[H][W];

typedef struct { int y, x; } Point;

typedef struct {
    Point b[S];
    int c;
    int y, x;
} Piece;

static Piece pieces[] = {
    { .b = {{0,0},{0,1},{0,2},{0,3}}, .c = 1 },  // I
    { .b = {{0,0},{1,0},{1,1},{1,2}}, .c = 2 },  // J
    { .b = {{0,2},{1,0},{1,1},{1,2}}, .c = 3 },  // L
    { .b = {{0,0},{0,1},{1,0},{1,1}}, .c = 4 },  // O
    { .b = {{0,1},{0,2},{1,0},{1,1}}, .c = 5 },  // S
    { .b = {{0,0},{0,1},{0,2},{1,1}}, .c = 6 },  // T
    { .b = {{0,0},{0,1},{1,1},{1,2}}, .c = 7 },  // Z
};

static Piece cur, next;
static int score, level, lines, over;


static Piece rand_piece(void) {
    return pieces[rand() % 7];
}

static void rotate(Point *p) {
    for (int i = 0; i < S; i++) {
        int t = p[i].y;
        p[i].y = p[i].x;
        p[i].x = -t;
    }
}

static int valid(Piece p) {
    for (int i = 0; i < S; i++) {
        int yy = p.y + p.b[i].y;
        int xx = p.x + p.b[i].x;
        if (xx < 0 || xx >= W || yy >= H) return 0;
        if (yy >= 0 && board[yy][xx]) return 0;
    }
    return 1;
}

static void place(void) {
    for (int i = 0; i < S; i++) {
        int yy = cur.y + cur.b[i].y;
        int xx = cur.x + cur.b[i].x;
        if (yy >= 0) board[yy][xx] = cur.c;
    }
}

static void clear_lines(void) {
    int cleared = 0;
    for (int y = H - 1; y >= 0; y--) {
        int full = 1;
        for (int x = 0; x < W && full; x++)
            if (!board[y][x]) full = 0;
        if (full) {
            for (int r = y; r > 0; r--)
                for (int x = 0; x < W; x++)
                    board[r][x] = board[r - 1][x];
            for (int x = 0; x < W; x++) board[0][x] = 0;
            y++;
            cleared++;
        }
    }
    if (cleared) {
        int pts[] = {0, 100, 300, 500, 800};
        score += (cleared <= 4) ? pts[cleared] : 800;
        lines += cleared;
        level = lines / 10 + 1;
    }
}

static void spawn(void) {
    cur = next;
    cur.y = -1;
    cur.x = W / 2 - 2;
    next = rand_piece();
    if (!valid(cur)) over = 1;
}

static void draw_piece(Piece p, int oy, int ox, int small) {
    for (int i = 0; i < S; i++) {
        int yy = oy + p.y + p.b[i].y;
        int xx = ox + (p.x + p.b[i].x) * 2;
        if (yy >= 0) {
            if (small) { attron(COLOR_PAIR(p.c)); mvaddch(yy, xx, '['); mvaddch(yy, xx + 1, ']'); attroff(COLOR_PAIR(p.c)); }
            else { attron(COLOR_PAIR(p.c) | A_REVERSE); mvaddch(yy, xx, ' '); mvaddch(yy, xx + 1, ' '); attroff(COLOR_PAIR(p.c) | A_REVERSE); }
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

    init_theme_pair(1, COLOR_CYAN, COLOR_CYAN);
    init_theme_pair(2, COLOR_BLUE, COLOR_BLUE);
    init_theme_pair(3, COLOR_YELLOW, COLOR_YELLOW);
    init_theme_pair(4, COLOR_WHITE, COLOR_WHITE);
    init_theme_pair(5, COLOR_GREEN, COLOR_GREEN);
    init_theme_pair(6, COLOR_MAGENTA, COLOR_MAGENTA);
    init_theme_pair(7, COLOR_RED, COLOR_RED);
    init_theme_pair(8, COLOR_WHITE, COLOR_BLACK);
    init_theme_pair(9, COLOR_CYAN, COLOR_BLACK);
    apply_theme_bg();

    int play_again = 1;
    while (play_again) {
        for (int y = 0; y < H; y++)
            for (int x = 0; x < W; x++)
                board[y][x] = 0;
        score = level = lines = over = 0;
        next = rand_piece();
        spawn();

        int oy = (LINES - H - 2) / 2;
        int ox = (COLS - W * 2 - 2) / 2;

        int tick = 0, ch;
        while (!over) {
            ch = getch();
            Piece tmp = cur;
            if (ch == 'q') { over = 1; play_again = 0; break; }
            if (ch == KEY_LEFT  || ch == 'a') { tmp.x--; if (valid(tmp)) cur.x--; }
            if (ch == KEY_RIGHT || ch == 'd') { tmp.x++; if (valid(tmp)) cur.x++; }
            if (ch == KEY_UP    || ch == 'w') { rotate(tmp.b); if (valid(tmp)) rotate(cur.b); }
            if (ch == KEY_DOWN  || ch == 's') { tmp.y++; if (valid(tmp)) cur.y++; }
            if (ch == ' ') {
                while (valid(tmp)) { cur.y++; tmp.y++; }
                cur.y--;
                place(); clear_lines(); spawn();
            }

            int delay = 20 - level;
            if (delay < 3) delay = 3;
            if (++tick % delay == 0) {
                tmp = cur; tmp.y++;
                if (valid(tmp)) cur.y++;
                else { place(); clear_lines(); spawn(); }
            }

            erase();

            // Board border
            attron(COLOR_PAIR(8));
            for (int x = 0; x < W * 2 + 2; x++) {
                mvaddch(oy, ox + x, '#');
                mvaddch(oy + H + 1, ox + x, '#');
            }
            for (int y = 0; y < H + 2; y++) {
                mvaddch(oy + y, ox, '#');
                mvaddch(oy + y, ox + W * 2 + 1, '#');
            }
            attroff(COLOR_PAIR(8));

            // Board content
            for (int y = 0; y < H; y++)
                for (int x = 0; x < W; x++)
                    if (board[y][x]) {
                        attron(COLOR_PAIR(board[y][x]) | A_REVERSE);
                        mvaddch(oy + y + 1, ox + x * 2 + 1, ' ');
                        mvaddch(oy + y + 1, ox + x * 2 + 2, ' ');
                        attroff(COLOR_PAIR(board[y][x]) | A_REVERSE);
                    }

            draw_piece(cur, oy + 1, ox + 1, 0);

            // Side panel
            int sx = ox + W * 2 + 4;
            attron(COLOR_PAIR(9) | A_BOLD);
            mvaddstr(oy + 2, sx, "TETRIS");
            attroff(COLOR_PAIR(9) | A_BOLD);

            attron(COLOR_PAIR(8));
            mvaddstr(oy + 4, sx, "NEXT:");
            draw_piece(next, oy + 5, sx, 1);

            char buf[32];
            snprintf(buf, sizeof buf, "SCORE: %d", score);
            mvaddstr(oy + 11, sx, buf);
            snprintf(buf, sizeof buf, "LEVEL: %d", level);
            mvaddstr(oy + 13, sx, buf);
            snprintf(buf, sizeof buf, "LINES: %d", lines);
            mvaddstr(oy + 15, sx, buf);

            mvaddstr(oy + 18, sx, "CONTROLS:");
            mvaddstr(oy + 19, sx, "<-  -> : move");
            mvaddstr(oy + 20, sx, "UP : rotate");
            mvaddstr(oy + 21, sx, "DOWN: down");
            mvaddstr(oy + 22, sx, "SPACE: drop");
            mvaddstr(oy + 23, sx, "Q: quit");
            attroff(COLOR_PAIR(8));

            refresh();
            usleep(50000);
        }

        if (play_again) {
            erase();
            attron(COLOR_PAIR(9) | A_BOLD);
            mvaddstr(oy + H / 2, ox + W / 2 - 5, "GAME OVER");
            attroff(COLOR_PAIR(9) | A_BOLD);
            attron(COLOR_PAIR(8));
            char buf[64];
            snprintf(buf, sizeof buf, "%s: %d  %s: %d", _("Score", "Pontuacao"), score, _("Level", "Nivel"), level);
            mvaddstr(oy + H / 2 + 2, ox + W / 2 - 8, buf);
            mvaddstr(oy + H / 2 + 4, ox + W / 2 - 12, _("Enter: play again | Q: quit", "Enter: jogar novamente | Q: sair"));
            attroff(COLOR_PAIR(8));
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
