/******************************************************************************
 *                                PONG
 *            Classic table tennis - 1 or 2 players!
 ******************************************************************************/
#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include "config.h"

#define W 60
#define H 22
#define PADDLE 4
#define WIN_SCORE 5
#define BALL_SPEED 60

#define C_BORDER 1
#define C_CENTER 2
#define C_BALL   3
#define C_P1     4
#define C_P2     5
#define C_SCORE  6
#define C_TITLE  7

typedef struct { int y, x; } Point;
typedef struct { Point pos; int dy, dx; int score; } Player;

static Player p1, p2;
static Point ball;
static int bdy, bdx;
static int mode; // 0 = vs AI, 1 = vs Player

static void init_game(int vs_ai) {
    mode = vs_ai;
    p1 = (Player){ { H / 2, 2 }, 0, 0, 0 };
    p2 = (Player){ { H / 2, W - 3 }, 0, 0, 0 };
    ball = (Point){ H / 2, W / 2 };
    bdy = (rand() % 2) ? 1 : -1;
    bdx = (rand() % 2) ? 1 : -1;
}

static void init_colors(void) {
    start_color();
    init_theme_pair(C_BORDER, COLOR_GREEN, COLOR_BLACK);
    init_theme_pair(C_CENTER, COLOR_WHITE, COLOR_BLACK);
    init_theme_pair(C_BALL, COLOR_YELLOW, COLOR_BLACK);
    init_theme_pair(C_P1, COLOR_RED, COLOR_BLACK);
    init_theme_pair(C_P2, COLOR_GREEN, COLOR_BLACK);
    init_theme_pair(C_SCORE, COLOR_WHITE, COLOR_BLACK);
    init_theme_pair(C_TITLE, COLOR_YELLOW, COLOR_BLACK);
    apply_theme_bg();
}

static void draw(void) {
    erase();

    // Borders - green
    attron(COLOR_PAIR(C_BORDER));
    for (int x = 0; x < W; x++) {
        mvaddch(0, x, '#');
        mvaddch(H + 1, x, '#');
    }
    for (int y = 0; y < H + 2; y++) {
        mvaddch(y, 0, '#');
        mvaddch(y, W - 1, '#');
    }
    attroff(COLOR_PAIR(C_BORDER));

    // Center line - dim white
    attron(COLOR_PAIR(C_CENTER) | A_DIM);
    for (int y = 1; y <= H; y++) {
        if (y % 2 == 0) mvaddch(y, W / 2, '|');
    }
    attroff(COLOR_PAIR(C_CENTER) | A_DIM);

    // Paddles (bold for visibility)
    attron(COLOR_PAIR(C_P1) | A_BOLD);
    for (int i = 0; i < PADDLE; i++)
        mvaddch(p1.pos.y + i, p1.pos.x, '|');
    attroff(COLOR_PAIR(C_P1) | A_BOLD);

    attron(COLOR_PAIR(C_P2) | A_BOLD);
    for (int i = 0; i < PADDLE; i++)
        mvaddch(p2.pos.y + i, p2.pos.x, '|');
    attroff(COLOR_PAIR(C_P2) | A_BOLD);

    // Ball - yellow bold
    attron(COLOR_PAIR(C_BALL) | A_BOLD);
    mvaddch(ball.y, ball.x, 'O');
    attroff(COLOR_PAIR(C_BALL) | A_BOLD);

    // Score - white bold
    attron(COLOR_PAIR(C_SCORE) | A_BOLD);
    char buf[16];
    snprintf(buf, sizeof buf, "%d", p1.score);
    mvaddstr(1, W / 2 - 6, buf);
    snprintf(buf, sizeof buf, "%d", p2.score);
    mvaddstr(1, W / 2 + 5, buf);
    attroff(COLOR_PAIR(C_SCORE) | A_BOLD);

    // Title & info
    attron(COLOR_PAIR(C_TITLE) | A_BOLD);
    if (mode == 0) mvaddstr(0, W / 2 - 10, " PONG - vs AI ");
    else mvaddstr(0, W / 2 - 12, " PONG - 2 Players ");
    attroff(COLOR_PAIR(C_TITLE) | A_BOLD);
    mvaddstr(H + 3, 2, "P1: W/S    P2: UP/DOWN");
    mvaddstr(H + 4, 2, _("R: restart | Q: quit", "R: restart | Q: sair"));
    mvaddstr(H + 5, 2, _("First to 5 wins!", "Primeiro a 5 vence!"));

    refresh();
}

static void reset_ball(void) {
    ball = (Point){ H / 2, W / 2 };
    bdy = (rand() % 2) ? 1 : -1;
    bdx = (rand() % 2) ? 1 : -1;
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
    init_colors();

    int play_again = 1;
    while (play_again) {
        // Mode selection
        int mode_sel = 0;
        int ch_mode;
        while (1) {
            erase();
            int oy = (LINES - 10) / 2;
            int ox = (COLS - 30) / 2;

            // Title box
            attron(COLOR_PAIR(C_TITLE) | A_BOLD);
            mvaddstr(oy, ox + 8, "+----------+");
            mvaddstr(oy + 1, ox + 8, "|   PONG   |");
            mvaddstr(oy + 2, ox + 8, "+----------+");
            attroff(COLOR_PAIR(C_TITLE) | A_BOLD);

            mvaddstr(oy + 4, ox, _("Select mode:", "Selecione o modo:"));
            if (mode_sel == 0) {
                attron(COLOR_PAIR(C_P1) | A_REVERSE);
                mvaddstr(oy + 6, ox + 4, _("1. vs Computer", "1. vs Computador"));
                attroff(COLOR_PAIR(C_P1) | A_REVERSE);
            } else {
                mvaddstr(oy + 6, ox + 4, _("1. vs Computer", "1. vs Computador"));
            }
            if (mode_sel == 1) {
                attron(COLOR_PAIR(C_P2) | A_REVERSE);
                mvaddstr(oy + 7, ox + 4, _("2. 2 Players", "2. 2 Jogadores"));
                attroff(COLOR_PAIR(C_P2) | A_REVERSE);
            } else {
                mvaddstr(oy + 7, ox + 4, _("2. 2 Players", "2. 2 Jogadores"));
            }
            mvaddstr(oy + 9, ox, _("Arrows: choose | Enter: confirm | Q: quit", "Setas: escolher | Enter: confirmar | Q: sair"));

            refresh();
            ch_mode = getch();
            if (ch_mode == 'q') { play_again = 0; break; }
            if (ch_mode == KEY_UP && mode_sel > 0) mode_sel--;
            if (ch_mode == KEY_DOWN && mode_sel < 1) mode_sel++;
            if (ch_mode == '\n') break;
        }
        if (!play_again) break;

        init_game(mode_sel == 0 ? 0 : 1);
        int over = 0, ch;
        int frame = 0;
        int ai_delay = 0;

        while (!over) {
            ch = getch();
            if (ch == 'q') { over = 1; play_again = 0; break; }
            if (ch == 'r') break;

            // Player 1 controls
            if ((ch == 'w' || ch == 'W') && p1.pos.y > 1) p1.pos.y--;
            if ((ch == 's' || ch == 'S') && p1.pos.y + PADDLE < H) p1.pos.y++;

            // Player 2 controls
            if (mode == 1) {
                if (ch == KEY_UP && p2.pos.y > 1) p2.pos.y--;
                if (ch == KEY_DOWN && p2.pos.y + PADDLE < H) p2.pos.y++;
            }

            // AI
            if (mode == 0) {
                ai_delay++;
                if (ai_delay % 3 == 0) {
                    if (ball.y < p2.pos.y + PADDLE / 2 && p2.pos.y > 1) p2.pos.y--;
                    else if (ball.y > p2.pos.y + PADDLE / 2 && p2.pos.y + PADDLE < H) p2.pos.y++;
                }
            }

            if (frame % 2 == 0) {
                // Move ball
                ball.y += bdy;
                ball.x += bdx;

                // Top/bottom bounce
                if (ball.y <= 1 || ball.y >= H) bdy = -bdy;

                // Paddle 1 collision
                if (ball.x == p1.pos.x + 1 && ball.y >= p1.pos.y && ball.y < p1.pos.y + PADDLE) {
                    bdx = -bdx;
                    ball.x++;
                }

                // Paddle 2 collision
                if (ball.x == p2.pos.x - 1 && ball.y >= p2.pos.y && ball.y < p2.pos.y + PADDLE) {
                    bdx = -bdx;
                    ball.x--;
                }

                // Score
                if (ball.x <= 0) { p2.score++; reset_ball(); usleep(300000); }
                if (ball.x >= W - 1) { p1.score++; reset_ball(); usleep(300000); }

                if (p1.score >= WIN_SCORE || p2.score >= WIN_SCORE) over = 1;
            }

            draw();
            usleep(BALL_SPEED * 1000);
            frame++;
        }

        if (play_again && ch != 'q') {
            erase();
            int oy = LINES / 2 - 4;
            int ox = COLS / 2 - 15;

            // Game over screen with colors
            attron(COLOR_PAIR(C_TITLE) | A_BOLD);
            mvaddstr(oy, ox + 6, "+-------------+");
            mvaddstr(oy + 1, ox + 6, "|  GAME OVER  |");
            mvaddstr(oy + 2, ox + 6, "+-------------+");
            attroff(COLOR_PAIR(C_TITLE) | A_BOLD);

            if (p1.score >= WIN_SCORE) {
                attron(COLOR_PAIR(C_P1) | A_BOLD);
                mvaddstr(oy + 4, ox + 4, _("Player 1 wins!", "Jogador 1 venceu!"));
                attroff(COLOR_PAIR(C_P1) | A_BOLD);
            } else {
                attron(COLOR_PAIR(C_P2) | A_BOLD);
                mvaddstr(oy + 4, ox + 4, _("Player 2 wins!", "Jogador 2 venceu!"));
                attroff(COLOR_PAIR(C_P2) | A_BOLD);
            }

            char buf[64];
            attron(COLOR_PAIR(C_SCORE));
            snprintf(buf, sizeof buf, _("Score: %d x %d", "Placar: %d x %d"), p1.score, p2.score);
            mvaddstr(oy + 6, ox + 6, buf);
            attroff(COLOR_PAIR(C_SCORE));

            mvaddstr(oy + 8, ox, _("Enter: play again | Q: quit", "Enter: jogar novamente | Q: sair"));
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
