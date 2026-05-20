/******************************************************************************
 *                              Q*BERT
 *        Hop on cubes to change their color - avoid the enemies!
 ******************************************************************************/
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <locale.h>
#include "config.h"

#define ROWS 7
#define CELLS 28
#define AREA_W 34
#define AREA_H 16
#define MAX_LIVES 3
#define COILY_INTERVAL 50
#define COILY_SPEED 6

typedef struct { int r, c; } Pos;

static int board[ROWS][ROWS];
static int qr, qc;
static int lives, score, level, changed, total_cubes;
static int game_over, won;

static int coily_alive, coily_r, coily_c, coily_timer, coily_move;
static int slick_alive, slick_r, slick_c, slick_timer;

static const char *cube_str[2] = {"\xe2\x97\x8b", "\xe2\x97\x8f"};

static void init_board(void) {
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c <= r; c++)
            board[r][c] = 0;
    changed = 0;
}

static void reset_qbert(void) {
    qr = 0; qc = 0;
}

static void init_level(void) {
    level++;
    init_board();
    reset_qbert();
    coily_alive = 0;
    slick_alive = 0;
    coily_timer = 0;
    slick_timer = 0;
    total_cubes = CELLS;
}

static void new_game(void) {
    lives = MAX_LIVES;
    score = 0;
    level = 0;
    game_over = 0;
    won = 0;
    init_level();
}

static int can_hop(int r, int c) {
    return r >= 0 && r < ROWS && c >= 0 && c <= r;
}

static int is_bottom(int r) {
    return r == ROWS - 1;
}

static int cube_x(int r, int c) {
    return (ROWS - 1 - r) * 2 + c * 4;
}

static int cube_y(int r) {
    return r;
}

static void spawn_coily(void) {
    if (coily_alive) return;
    coily_alive = 1;
    coily_r = 0;
    coily_c = 0;
    coily_move = 0;
}

static void update_coily(void) {
    if (!coily_alive) return;
    coily_move++;

    /* Also move when Q*bert hops (moved from game loop) */
    if (coily_move >= COILY_SPEED) {
        coily_move = 0;
        int dir = rand() % 2;
        int nr = coily_r + 1;
        int nc = coily_c + dir;
        if (nr >= ROWS) {
            coily_alive = 0;
            score += 100;
            return;
        }
        coily_r = nr;
        coily_c = nc;

        if (coily_r == qr && coily_c == qc) {
            lives--;
            if (lives <= 0) { game_over = 1; return; }
            reset_qbert();
        }
    }
}

static void update_slick(void) {
    if (!slick_alive) return;

    int r = rand() % ROWS;
    int c = rand() % (r + 1);
    if (board[r][c] == 1 && !(r == qr && c == qc)
        && !(coily_alive && r == coily_r && c == coily_c)) {
        board[r][c] = 0;
        changed--;
    }
}

static int hop(int dr, int dc) {
    if (game_over) return 0;
    int nr = qr + dr;
    int nc = qc + dc;

    if (is_bottom(qr) && (qr + dr >= ROWS)) {
        /* Fall off the pyramid */
        lives--;
        if (lives <= 0) { game_over = 1; return 1; }
        reset_qbert();
        return 1;
    }

    if (!can_hop(nr, nc)) return 0;

    qr = nr; qc = nc;

    if (!board[qr][qc]) {
        board[qr][qc] = 1;
        changed++;
        score += 25;
    }

    if (changed >= total_cubes) {
        if (level >= 10) { game_over = 1; won = 1; return 1; }
        score += level * 100 + lives * 50;
        init_level();
        return 1;
    }

    /* Check enemy collision */
    if (coily_alive && qr == coily_r && qc == coily_c) {
        lives--;
        if (lives <= 0) { game_over = 1; return 1; }
        reset_qbert();
    }

    return 1;
}

static void draw(int ox, int oy) {
    erase();
    char buf[64];

    /* Title / HUD */
    attron(COLOR_PAIR(5) | A_BOLD);
    mvaddstr(oy, ox, _("Q*BERT", "Q*BERT"));
    attroff(COLOR_PAIR(5) | A_BOLD);

    snprintf(buf, 64, _("Lv %d  Score: %d  Lives: %d",
                         "Lv %d  Pontos: %d  Vidas: %d"),
             level, score, lives);
    attron(COLOR_PAIR(2));
    mvaddstr(oy + 1, ox, buf);
    attroff(COLOR_PAIR(2));

    snprintf(buf, 64, _("Cubes: %d/%d", "Cubos: %d/%d"), changed, total_cubes);
    attron(COLOR_PAIR(5));
    mvaddstr(oy + 2, ox, buf);
    attroff(COLOR_PAIR(5));

    /* Draw pyramid */
    int py = oy + 4;
    int px = ox + 2;

    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c <= r; c++) {
            int x = px + cube_x(r, c);
            int y = py + cube_y(r);

            if (r == qr && c == qc) {
                attron(COLOR_PAIR(2) | A_BOLD);
                mvaddstr(y, x, _("@ ", "@ "));
                attroff(COLOR_PAIR(2) | A_BOLD);
            } else if (coily_alive && r == coily_r && c == coily_c) {
                attron(COLOR_PAIR(4) | A_BOLD);
                mvaddstr(y, x, _("S ", "S "));
                attroff(COLOR_PAIR(4) | A_BOLD);
            } else {
                int col = board[r][c] ? 3 : 6;
                attron(COLOR_PAIR(col) | (board[r][c] ? A_BOLD : A_DIM));
                mvaddstr(y, x, cube_str[board[r][c]]);
                attroff(COLOR_PAIR(col) | (board[r][c] ? A_BOLD : A_DIM));
            }
        }
    }

    /* Instructions */
    attron(COLOR_PAIR(6) | A_DIM);
    int iy = py + ROWS + 2;
    mvaddstr(iy, ox,
             _("U: up-left  I: up-right  J: down-left  K: down-right",
               "U: cima-esq  I: cima-dir  J: baixo-esq  K: baixo-dir"));
    mvaddstr(iy + 1, ox, _("Q: quit", "Q: sair"));
    attroff(COLOR_PAIR(6) | A_DIM);

    if (game_over) {
        attron(COLOR_PAIR(4) | A_BOLD);
        mvaddstr(py + ROWS / 2, px + 2,
                 won ? _("YOU WIN!", "VOCE VENCEU!")
                     : _("GAME OVER", "FIM DE JOGO"));
        attroff(COLOR_PAIR(4) | A_BOLD);

        snprintf(buf, 64, _("Final score: %d", "Pontuacao: %d"), score);
        attron(COLOR_PAIR(2));
        mvaddstr(py + ROWS / 2 + 2, px + 4, buf);
        attroff(COLOR_PAIR(2));

        attron(COLOR_PAIR(6) | A_DIM);
        mvaddstr(py + ROWS / 2 + 4, px,
                 _("Enter: play again  Q: quit",
                   "Enter: jogar novamente  Q: sair"));
        attroff(COLOR_PAIR(6) | A_DIM);
    }

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
            int ox = (COLS - AREA_W) / 2 < 1 ? 1 : (COLS - AREA_W) / 2;
            int oy = (LINES - AREA_H) / 2 < 1 ? 1 : (LINES - AREA_H) / 2;

            if (!game_over) {

                /* Spawn Coily */
                coily_timer++;
                if (coily_timer >= COILY_INTERVAL && level > 1) {
                    coily_timer = 0;
                    spawn_coily();
                }
                update_coily();
                if (game_over) { draw(ox, oy); goto input; }

                /* Slick appears at higher levels */
                if (level > 2 && rand() % 200 == 0) {
                    slick_alive = 1;
                    slick_r = rand() % ROWS;
                    slick_c = rand() % (slick_r + 1);
                }
                if (level > 2 && rand() % 80 == 0)
                    update_slick();
            }

input:
            draw(ox, oy);

            int ch = getch();
            if (ch == 'q') { play = 0; break; }

            if (game_over) {
                if (ch == '\n' || ch == ' ') break;
                usleep(16000);
                continue;
            }

            if (ch == 'u' || ch == 'U') hop(-1, -1);
            else if (ch == 'i' || ch == 'I') hop(-1, 0);
            else if (ch == 'j' || ch == 'J') hop(1, 0);
            else if (ch == 'k' || ch == 'K') hop(1, 1);

            usleep(33000);
        }
        if (!play) break;
    }

    endwin();
    return 0;
}
