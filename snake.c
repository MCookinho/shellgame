/******************************************************************************
 *                                SNAKE
 *                    Classic snake game with progressive speed
 ******************************************************************************/
#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#include "config.h"
#pragma GCC diagnostic pop

#define MAX 1024
#define WIN_W 40
#define WIN_H 20

typedef struct { int y, x; } Point;

static Point snake[MAX];
static int len;
static int dy, dx;
static Point food;
static int score;
static int speed;

static void spawn_food(void) {
    do {
        food.y = rand() % WIN_H;
        food.x = rand() % WIN_W;
    } while (food.y == 0 && food.x == 0);
    for (int i = 0; i < len; i++)
        if (snake[i].y == food.y && snake[i].x == food.x)
            spawn_food();
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

    init_theme_pair(1, COLOR_GREEN, COLOR_BLACK);   // snake
    init_theme_pair(2, COLOR_RED, COLOR_BLACK);     // food
    init_theme_pair(3, COLOR_YELLOW, COLOR_BLACK);  // head
    init_theme_pair(4, COLOR_WHITE, COLOR_BLACK);   // border
    init_theme_pair(5, COLOR_CYAN, COLOR_BLACK);    // text
    apply_theme_bg();

    int ch, play_again = 1;

    while (play_again) {
        len = 4;
        dx = 1; dy = 0;
        score = 0;
        speed = 120;
        int oy = (LINES - WIN_H - 2) / 2;
        int ox = (COLS - WIN_W - 2) / 2;

        snake[0] = (Point){ WIN_H / 2, WIN_W / 2 };
        for (int i = 1; i < len; i++)
            snake[i] = (Point){ snake[0].y, snake[0].x - i };

        spawn_food();

        int running = 1;
        while (running) {
            ch = getch();
            if (ch == 'q') { running = 0; play_again = 0; break; }
            if ((ch == KEY_UP    || ch == 'w') && dy !=  1) { dy = -1; dx =  0; }
            if ((ch == KEY_DOWN  || ch == 's') && dy != -1) { dy =  1; dx =  0; }
            if ((ch == KEY_LEFT  || ch == 'a') && dx !=  1) { dy =  0; dx = -1; }
            if ((ch == KEY_RIGHT || ch == 'd') && dx != -1) { dy =  0; dx =  1; }

            Point head = { snake[0].y + dy, snake[0].x + dx };

            if (head.x < 0 || head.x >= WIN_W || head.y < 0 || head.y >= WIN_H)
                { running = -1; break; }
            for (int i = 0; i < len; i++)
                if (head.y == snake[i].y && head.x == snake[i].x)
                    { running = -1; break; }
            if (running == -1) break;

            for (int i = len; i > 0; i--)
                snake[i] = snake[i - 1];
            snake[0] = head;

            if (head.y == food.y && head.x == food.x) {
                len++;
                score += 10;
                if (speed > 40) speed -= 2;
                spawn_food();
            }

            erase();

            // Border
            attron(COLOR_PAIR(4));
            for (int x = 0; x < WIN_W + 2; x++) {
                mvaddch(oy, ox + x, '#');
                mvaddch(oy + WIN_H + 1, ox + x, '#');
            }
            for (int y = 0; y < WIN_H + 2; y++) {
                mvaddch(oy + y, ox, '#');
                mvaddch(oy + y, ox + WIN_W + 1, '#');
            }
            attroff(COLOR_PAIR(4));

            // Food
            attron(COLOR_PAIR(2) | A_BOLD);
            mvaddch(oy + food.y + 1, ox + food.x + 1, '@');
            attroff(COLOR_PAIR(2) | A_BOLD);

            // Snake body
            attron(COLOR_PAIR(1));
            for (int i = 1; i < len; i++)
                mvaddch(oy + snake[i].y + 1, ox + snake[i].x + 1, 'o');
            attroff(COLOR_PAIR(1));

            // Snake head
            attron(COLOR_PAIR(3) | A_BOLD);
            mvaddch(oy + snake[0].y + 1, ox + snake[0].x + 1, 'O');
            attroff(COLOR_PAIR(3) | A_BOLD);

            // Score & info
            attron(COLOR_PAIR(5));
            mvaddstr(oy - 1, ox, "SNAKE");
            char buf[64];
            snprintf(buf, sizeof buf, _("Score: %d  Length: %d", "Pontuacao: %d  Tamanho: %d"), score, len);
            mvaddstr(oy + WIN_H + 3, ox, buf);
            mvaddstr(oy + WIN_H + 4, ox, _("WASD/Arrows: move | Q: quit", "WASD/Setas: mover | Q: sair"));
            attroff(COLOR_PAIR(5));

            refresh();
            usleep(speed * 1000);
        }

        erase();
        if (running == -1) {
            attron(COLOR_PAIR(2) | A_BOLD);
            mvaddstr(oy + WIN_H / 2, ox + WIN_W / 2 - 5, "GAME OVER");
            attroff(COLOR_PAIR(2) | A_BOLD);
            attron(COLOR_PAIR(5));
            char buf[64];
            const char *final_fmt = _("Final score: %d", "Pontuacao final: %d");
            snprintf(buf, sizeof buf, final_fmt, score);
            mvaddstr(oy + WIN_H / 2 + 2, ox + WIN_W / 2 - 6, buf);
            mvaddstr(oy + WIN_H / 2 + 4, ox + WIN_W / 2 - 10, _("Enter: play again | Q: quit", "Enter: jogar novamente | Q: sair"));
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
