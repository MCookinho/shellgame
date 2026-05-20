/******************************************************************************
 *                           SPACE INVADERS
 *              Defend Earth from the alien invasion!
 ******************************************************************************/
#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include "config.h"

#define W 40
#define H 22
#define ALIEN_COLS 8
#define MAX_ALIEN_ROWS 6
#define MAX_BULLETS 5
#define MAX_BOMBS 12
#define MAX_LEVEL 6

typedef struct { int y, x, active; } Bullet;
typedef struct { int y, x, active; } Bomb;

static int player_x;
static Bullet bullets[MAX_BULLETS];
static Bomb bombs[MAX_BOMBS];
static int aliens[MAX_ALIEN_ROWS][ALIEN_COLS];
static int alien_dir, alien_speed, alien_move_counter;
static int alien_rows, bomb_rate;
static int score, lives, level, won, over;
static int frame;

static const char *alien_top[] = {
    "/^\\", "|-|", "<O>", "[o]", "\\W/", "{X}"
};
static const char *alien_bot[] = {
    "\\_/", "|_|", "( )", "/ \\", "(O)", "{X}"
};
static int alien_color1[] = { 3, 6, 2, 5, 1, 7 };
static int alien_color2[] = { 5, 4, 3, 1, 2, 8 };

static void get_level_params(int lv, int *rows, int *speed, int *bombs) {
    switch (lv) {
        case 1: *rows = 4; *speed = 22; *bombs = 30; break;
        case 2: *rows = 4; *speed = 18; *bombs = 26; break;
        case 3: *rows = 5; *speed = 16; *bombs = 22; break;
        case 4: *rows = 5; *speed = 14; *bombs = 18; break;
        case 5: *rows = 6; *speed = 12; *bombs = 14; break;
        default: *rows = 6; *speed = 10; *bombs = 10; break;
    }
}

static void init_game(void) {
    player_x = W / 2;
    score = 0; lives = 3; level = 1; won = 0; over = 0; frame = 0;
    alien_dir = 1; alien_move_counter = 0;
    get_level_params(level, &alien_rows, &alien_speed, &bomb_rate);

    for (int i = 0; i < MAX_ALIEN_ROWS; i++)
        for (int j = 0; j < ALIEN_COLS; j++)
            aliens[i][j] = (i < alien_rows) ? 1 : 0;

    for (int i = 0; i < MAX_BULLETS; i++) bullets[i].active = 0;
    for (int i = 0; i < MAX_BOMBS; i++) bombs[i].active = 0;
}

static void next_level(void) {
    level++;
    if (level > MAX_LEVEL) { won = 1; return; }
    get_level_params(level, &alien_rows, &alien_speed, &bomb_rate);
    alien_dir = 1; alien_move_counter = 0;
    for (int i = 0; i < MAX_ALIEN_ROWS; i++)
        for (int j = 0; j < ALIEN_COLS; j++)
            aliens[i][j] = (i < alien_rows) ? 1 : 0;
    for (int i = 0; i < MAX_BULLETS; i++) bullets[i].active = 0;
    for (int i = 0; i < MAX_BOMBS; i++) bombs[i].active = 0;
    won = 0; over = 0;
}

static void spawn_bullet(void) {
    for (int i = 0; i < MAX_BULLETS; i++)
        if (!bullets[i].active) {
            bullets[i] = (Bullet){ H - 2, player_x, 1 };
            break;
        }
}

static void spawn_bomb(void) {
    int candidates[alien_rows * ALIEN_COLS][2];
    int n = 0;
    for (int j = 0; j < ALIEN_COLS; j++) {
        int bottom = -1;
        for (int i = alien_rows - 1; i >= 0; i--)
            if (aliens[i][j]) { bottom = i; break; }
        if (bottom >= 0) { candidates[n][0] = bottom; candidates[n][1] = j; n++; }
    }
    if (n == 0) return;
    int idx = rand() % n;
    int r = candidates[idx][0], c = candidates[idx][1];
    for (int i = 0; i < MAX_BOMBS; i++)
        if (!bombs[i].active) {
            bombs[i] = (Bomb){ 2 + r * 2, 2 + c * 4 + 1 + (rand() % 3 - 1), 1 };
            break;
        }
}

static void move_aliens(void) {
    alien_move_counter++;
    if (alien_move_counter < alien_speed) return;
    alien_move_counter = 0;

    int hit_edge = 0;
    for (int i = 0; i < alien_rows && !hit_edge; i++)
        for (int j = 0; j < ALIEN_COLS && !hit_edge; j++)
            if (aliens[i][j]) {
                int nx = 2 + j * 4 + alien_dir;
                if (nx <= 1 || nx >= W - 3) hit_edge = 1;
            }

    if (hit_edge) {
        alien_dir *= -1;
        for (int i = 0; i < alien_rows; i++)
            for (int j = 0; j < ALIEN_COLS; j++)
                if (aliens[i][j]) {
                    int ny = 2 + i * 2 + 1;
                    if (ny >= H - 4) { over = 1; return; }
                }
    }

    for (int i = alien_rows - 1; i >= 0; i--)
        for (int j = 0; j < ALIEN_COLS; j++)
            if (aliens[i][j]) {
                int ny = 2 + i * 2;
                if (hit_edge) ny++;
                // Check if alien reached player
                if (ny >= H - 3) over = 1;
            }
}

static void move_bullets(void) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) continue;
        bullets[i].y--;
        // Check alien collision
        int hit = 0;
        for (int r = 0; r < alien_rows && !hit; r++)
            for (int c = 0; c < ALIEN_COLS && !hit; c++)
                if (aliens[r][c]) {
                    int ay = 2 + r * 2, ax = 2 + c * 4;
                    if (bullets[i].y >= ay && bullets[i].y <= ay + 1 &&
                        bullets[i].x >= ax && bullets[i].x <= ax + 3) {
                        aliens[r][c] = 0;
                        bullets[i].active = 0;
                        score += 10 * level;
                        hit = 1;
                    }
                }
        if (hit) continue;
        if (bullets[i].y <= 0) bullets[i].active = 0;
    }
}

static void move_bombs(void) {
    for (int i = 0; i < MAX_BOMBS; i++) {
        if (!bombs[i].active) continue;
        bombs[i].y++;
        if (bombs[i].y >= H - 1) { bombs[i].active = 0; continue; }
        if (bombs[i].y == H - 2 && bombs[i].x >= player_x - 1 && bombs[i].x <= player_x + 1) {
            lives--;
            bombs[i].active = 0;
            if (lives <= 0) over = 1;
        }
    }
}

static void check_win(void) {
    int remaining = 0;
    for (int i = 0; i < alien_rows; i++)
        for (int j = 0; j < ALIEN_COLS; j++)
            if (aliens[i][j]) remaining++;
    if (remaining == 0) won = 1;
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

    init_theme_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_theme_pair(2, COLOR_RED, COLOR_BLACK);
    init_theme_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_theme_pair(4, COLOR_CYAN, COLOR_BLACK);
    init_theme_pair(5, COLOR_MAGENTA, COLOR_BLACK);
    init_theme_pair(6, COLOR_WHITE, COLOR_BLACK);
    init_theme_pair(7, COLOR_WHITE, COLOR_BLACK);
    init_theme_pair(8, COLOR_RED, COLOR_BLACK);
    apply_theme_bg();

    int play_again = 1;
    while (play_again) {
        init_game();

        int ch;
        int oy = (LINES - H - 2) / 2;
        int ox = (COLS - W) / 2;

        while (1) {
            while (!over && !won) {
                ch = getch();
                if (ch == 'q') { over = 1; play_again = 0; break; }

                if ((ch == KEY_LEFT || ch == 'a') && player_x > 3) player_x--;
                if ((ch == KEY_RIGHT || ch == 'd') && player_x < W - 4) player_x++;
                if (ch == ' ' || ch == '\n') spawn_bullet();

                move_aliens();

                if (frame % bomb_rate == 0 && frame > 0)
                    spawn_bomb();

                if (frame % 2 == 0) move_bullets();
                if (frame % 2 == 0) move_bombs();
                check_win();

                erase();

                attron(COLOR_PAIR(4));
                for (int x = 0; x < W; x++) {
                    mvaddch(oy, ox + x, '-');
                    mvaddch(oy + H + 1, ox + x, '-');
                }
                for (int y = 0; y < H + 2; y++) {
                    mvaddch(oy + y, ox, '|');
                    mvaddch(oy + y, ox + W - 1, '|');
                }
                attroff(COLOR_PAIR(4));

                int lv_idx = (level - 1) % MAX_LEVEL;
                for (int r = 0; r < alien_rows; r++)
                    for (int c = 0; c < ALIEN_COLS; c++)
                        if (aliens[r][c]) {
                            int ay = oy + 2 + r * 2;
                            int ax = ox + 2 + c * 4;
                            int color = (r % 2 == 0) ? alien_color1[lv_idx] : alien_color2[lv_idx];
                            int attr = COLOR_PAIR(color) | A_BOLD;
                            if (level == MAX_LEVEL) attr |= A_BLINK;
                            attron(attr);
                            mvaddstr(ay, ax, alien_top[lv_idx]);
                            mvaddstr(ay + 1, ax, alien_bot[lv_idx]);
                            attroff(attr);
                        }

                attron(COLOR_PAIR(1) | A_BOLD);
                mvaddstr(oy + H - 2, ox + player_x - 1, "/A\\");
                attroff(COLOR_PAIR(1) | A_BOLD);

                attron(COLOR_PAIR(4) | A_BOLD);
                for (int i = 0; i < MAX_BULLETS; i++)
                    if (bullets[i].active)
                        mvaddch(oy + bullets[i].y, ox + bullets[i].x, '|');
                attroff(COLOR_PAIR(4) | A_BOLD);

                attron(COLOR_PAIR(2) | A_BOLD);
                for (int i = 0; i < MAX_BOMBS; i++)
                    if (bombs[i].active)
                        mvaddch(oy + bombs[i].y, ox + bombs[i].x, '*');
                attroff(COLOR_PAIR(2) | A_BOLD);

                attron(COLOR_PAIR(6));
                char buf[64];
                snprintf(buf, sizeof buf, "Score: %d  Lives: %d  Level: %d/%d",
                         score, lives, level, MAX_LEVEL);
                mvaddstr(oy + H + 3, ox, buf);
                mvaddstr(oy + H + 4, ox, _("<- ->: move | Space: shoot | Q: quit", "<- ->: mover | Espaco: atirar | Q: sair"));
                attroff(COLOR_PAIR(6));

                refresh();
                usleep(40000);
                frame++;
            }

            if (!play_again) break;

            if (won && level < MAX_LEVEL) {
                erase();
                attron(COLOR_PAIR(3) | A_BOLD);
                char msg[32];
                snprintf(msg, sizeof msg, _("LEVEL %d CLEAR!", "NIVEL %d CONCLUIDO!"), level);
                mvaddstr(oy + H / 2, ox + W/2 - 8, msg);
                attroff(COLOR_PAIR(3) | A_BOLD);
                attron(COLOR_PAIR(6));
                mvaddstr(oy + H / 2 + 2, ox + W/2 - 10,
                         _("Get ready for next level...", "Prepare-se para o proximo nivel..."));
                attroff(COLOR_PAIR(6));
                refresh();
                nodelay(stdscr, FALSE);
                getch();
                nodelay(stdscr, TRUE);
                next_level();
                continue;
            }
            break;
        }

        if (play_again) {
            erase();
            if (won) {
                attron(A_BOLD | COLOR_PAIR(3));
                mvaddstr(oy + H / 2, ox + 8, _("YOU WIN!", "VOCE VENCEU!"));
                attroff(A_BOLD | COLOR_PAIR(3));
                attron(COLOR_PAIR(6));
                char buf[64];
                snprintf(buf, sizeof buf, _("Final Score: %d", "Pontuacao Final: %d"), score);
                mvaddstr(oy + H / 2 + 2, ox + 10, buf);
            } else {
                attron(A_BOLD | COLOR_PAIR(2));
                mvaddstr(oy + H / 2, ox + 8, "GAME OVER!");
                attroff(A_BOLD | COLOR_PAIR(2));
                attron(COLOR_PAIR(6));
                char buf[64];
                snprintf(buf, sizeof buf, "Score: %d", score);
                mvaddstr(oy + H / 2 + 2, ox + 10, buf);
            }
            mvaddstr(oy + H / 2 + 4, ox + 4, _("Enter: play again | Q: quit", "Enter: jogar novamente | Q: sair"));
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
