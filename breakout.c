/******************************************************************************
 *                              BREAKOUT
 *         Break all the bricks with your paddle and ball!
 ******************************************************************************/
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <locale.h>
#include "config.h"

#define AREA_W 40
#define AREA_H 22
#define BRICK_ROWS 6
#define BRICK_COLS 10
#define BRICK_W 3
#define BRICK_GAP 1
#define PADDLE_W 7
#define BALL_CH '+'
#define MAX_LIVES 3

typedef struct {
    int alive;
} Brick;

static Brick bricks[BRICK_ROWS][BRICK_COLS];
static int px, bx, by, bvx, bvy;
static int lives, score, level, ball_live;
static int game_over, won, total_bricks;
static int ball_timer, ball_delay;
static int pdx; /* paddle direction from input */

static int brick_colors[BRICK_ROWS] = {4, 2, 5, 3, 1, 6};

static void init_level(void) {
    level++;
    total_bricks = 0;
    for (int r = 0; r < BRICK_ROWS; r++) {
        for (int c = 0; c < BRICK_COLS; c++) {
            bricks[r][c].alive = 1;
            total_bricks++;
        }
    }
    px = AREA_W / 2 - PADDLE_W / 2;
    bx = AREA_W / 2;
    by = AREA_H - 3;
    bvx = 1;
    bvy = -1;
    ball_live = 1;
    pdx = 0;
    ball_delay = 6;
    ball_timer = 0;
}

static void new_game(void) {
    lives = MAX_LIVES;
    score = 0;
    level = 0;
    game_over = 0;
    won = 0;
    init_level();
}

static void launch_ball(void) {
    bx = px + PADDLE_W / 2;
    by = AREA_H - 3;
    bvx = (rand() % 2) ? 1 : -1;
    bvy = -1;
    ball_live = 1;
}

static void update_ball(void) {
    if (!ball_live || game_over) return;

    ball_timer++;
    if (ball_timer < ball_delay) return;
    ball_timer = 0;

    bx += bvx;
    by += bvy;

    /* Wall bounce */
    if (bx <= 0) { bx = 0; bvx = 1; }
    if (bx >= AREA_W - 1) { bx = AREA_W - 2; bvx = -1; }
    if (by <= 1) { by = 1; bvy = 1; }

    /* Paddle bounce */
    if (by >= AREA_H - 2 && by < AREA_H
        && bx >= px && bx < px + PADDLE_W) {
        by = AREA_H - 3;
        bvy = -1;
        int hit = bx - px;
        if (hit < PADDLE_W / 3) bvx = -1;
        else if (hit < PADDLE_W * 2 / 3) bvx = (bvx > 0) ? 1 : -1;
        else bvx = 1;
    }

    /* Bottom */
    if (by >= AREA_H) {
        ball_live = 0;
        lives--;
        if (lives <= 0) { game_over = 1; return; }
        launch_ball();
        return;
    }

    /* Brick collision */
    int br = (by - 3) / 1;
    int bc = bx / (BRICK_W + BRICK_GAP);
    if (br >= 0 && br < BRICK_ROWS
        && bc >= 0 && bc < BRICK_COLS
        && bricks[br][bc].alive) {
        bricks[br][bc].alive = 0;
        total_bricks--;
        score += (BRICK_ROWS - br) * 10;
        bvy = -bvy;
    }

    if (total_bricks <= 0) {
        if (level >= 5) { game_over = 1; won = 1; return; }
        score += lives * 50;
        init_level();
    }
}

static void draw(int ox, int oy) {
    erase();
    char buf[64];

    /* HUD */
    attron(COLOR_PAIR(5) | A_BOLD);
    mvaddstr(oy, ox, _("BREAKOUT", "BREAKOUT"));
    attroff(COLOR_PAIR(5) | A_BOLD);

    snprintf(buf, 64, _("Lv: %d  Score: %d  Lives: %d  Bricks: %d",
                         "Lv: %d  Pontos: %d  Vidas: %d  Tijolos: %d"),
             level, score, lives, total_bricks);
    attron(COLOR_PAIR(2));
    mvaddstr(oy + 1, ox, buf);
    attroff(COLOR_PAIR(2));

    /* Border */
    int ay = oy + 3;
    int ax = ox;
    for (int c = 0; c < AREA_W; c++) {
        mvaddch(ay, ax + c, '-');
        mvaddch(ay + AREA_H, ax + c, '-');
    }
    for (int r = 0; r <= AREA_H; r++)
        mvaddch(ay + r, ax, '|');
    for (int r = 0; r <= AREA_H; r++)
        mvaddch(ay + r, ax + AREA_W - 1, '|');

    /* Bricks */
    for (int r = 0; r < BRICK_ROWS; r++) {
        for (int c = 0; c < BRICK_COLS; c++) {
            if (!bricks[r][c].alive) continue;
            int x = ax + 1 + c * (BRICK_W + BRICK_GAP);
            int y = ay + 2 + r;
            attron(COLOR_PAIR(brick_colors[r]) | A_BOLD);
            mvaddch(y, x, '[');
            mvaddch(y, x + 1, '#');
            mvaddch(y, x + 2, ']');
            attroff(COLOR_PAIR(brick_colors[r]) | A_BOLD);
        }
    }

    /* Ball */
    if (ball_live) {
        attron(COLOR_PAIR(3) | A_BOLD);
        mvaddch(ay + by, ax + bx, BALL_CH);
        attroff(COLOR_PAIR(3) | A_BOLD);
    }

    /* Paddle */
    attron(COLOR_PAIR(4) | A_BOLD);
    for (int i = 0; i < PADDLE_W; i++)
        mvaddch(ay + AREA_H - 1, ax + px + i, '=');
    attroff(COLOR_PAIR(4) | A_BOLD);

    /* Game over */
    if (game_over) {
        attron(COLOR_PAIR(4) | A_BOLD);
        mvaddstr(ay + AREA_H / 2 - 1, ax + AREA_W / 2 - 6,
                 won ? _("YOU WIN!", "VENCEU!")
                     : _("GAME OVER", "FIM DE JOGO"));
        attroff(COLOR_PAIR(4) | A_BOLD);
        snprintf(buf, 64, _("Final score: %d", "Pontuacao: %d"), score);
        attron(COLOR_PAIR(2));
        mvaddstr(ay + AREA_H / 2 + 1, ax + AREA_W / 2 - 8, buf);
        attroff(COLOR_PAIR(2));
        attron(COLOR_PAIR(6) | A_DIM);
        mvaddstr(ay + AREA_H / 2 + 3, ax + AREA_W / 2 - 10,
                 _("Enter: play again  Q: quit",
                   "Enter: jogar novamente  Q: sair"));
        attroff(COLOR_PAIR(6) | A_DIM);
    }

    /* Instructions */
    attron(COLOR_PAIR(6) | A_DIM);
    mvaddstr(ay + AREA_H + 2, ax,
             _("Arrows/WASD: move  Q: quit",
               "Setas/WASD: mover  Q: sair"));
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
    init_theme_pair(5, COLOR_MAGENTA, COLOR_BLACK);
    init_theme_pair(6, COLOR_WHITE, COLOR_BLACK);
    apply_theme_bg();

    int play = 1;
    while (play) {
        new_game();

        while (1) {
            int ox = (COLS - AREA_W - 2) / 2 < 1 ? 1 : (COLS - AREA_W - 2) / 2;
            int oy = (LINES - 28) / 2 < 1 ? 1 : (LINES - 28) / 2;

            if (!game_over) {
                /* Move paddle */
                px += pdx;
                if (px < 1) px = 1;
                if (px + PADDLE_W > AREA_W - 1) px = AREA_W - 1 - PADDLE_W;

                update_ball();
            }

            draw(ox, oy);

            int ch = getch();
            if (ch == 'q') { play = 0; break; }

            if (game_over) {
                if (ch == '\n' || ch == ' ') break;
                usleep(16000);
                continue;
            }

            pdx = 0;
            if (ch == KEY_LEFT || ch == 'a') pdx = -1;
            else if (ch == KEY_RIGHT || ch == 'd') pdx = 1;
            else if (ch == ' ') launch_ball();

            usleep(16000);
        }
        if (!play) break;
    }

    endwin();
    return 0;
}
