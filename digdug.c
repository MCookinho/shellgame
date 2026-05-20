/******************************************************************************
 *                               DIG DUG
 *         Dig tunnels and defeat enemies with your air pump!
 ******************************************************************************/
#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include "config.h"

#define W 28
#define H 18
#define MAX_ENEMIES 6
#define PUMP_MAX 5

typedef struct { int x, y, dir, hp, active; } Enemy;

static int grid[H][W];
static Enemy enemies[MAX_ENEMIES];
static int px, py;
static int score, lives, level, over, won;
static int frame, pump_timer, pump_dir;
static int enemy_spawn_timer;

static void init_game(void) {
    score = 0; lives = 3; level = 1; over = 0; won = 0;
    frame = 0; pump_timer = 0; pump_dir = 0;
    enemy_spawn_timer = 0;
    px = W / 2; py = H / 2;

    // Fill grid with dirt
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
            grid[y][x] = 1;

    // Clear some starting area
    for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++)
            if (py + dy >= 0 && py + dy < H && px + dx >= 0 && px + dx < W)
                grid[py + dy][px + dx] = 0;

    for (int i = 0; i < MAX_ENEMIES; i++) enemies[i].active = 0;
}

static void spawn_enemy(void) {
    for (int i = 0; i < MAX_ENEMIES; i++)
        if (!enemies[i].active) {
            int ex, ey;
            int side = rand() % 4;
            if (side == 0) { ex = 1; ey = 1; }
            else if (side == 1) { ex = W - 2; ey = 1; }
            else if (side == 2) { ex = 1; ey = H - 2; }
            else { ex = W - 2; ey = H - 2; }
            enemies[i] = (Enemy){ ex, ey, 1, PUMP_MAX, 1 };
            break;
        }
}

static void move_enemies(void) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;
        int dx = 0, dy = 0;
        // Move toward player with some randomness
        if (rand() % 3 != 0) {
            if (abs(px - enemies[i].x) > abs(py - enemies[i].y))
                dx = (px > enemies[i].x) ? 1 : -1;
            else
                dy = (py > enemies[i].y) ? 1 : -1;
        } else {
            int r = rand() % 4;
            if (r == 0) dx = 1;
            else if (r == 1) dx = -1;
            else if (r == 2) dy = 1;
            else dy = -1;
        }
        int nx = enemies[i].x + dx;
        int ny = enemies[i].y + dy;
        if (nx >= 0 && nx < W && ny >= 0 && ny < H) {
            enemies[i].x = nx;
            enemies[i].y = ny;
        }
        // Check collision with player
        if (enemies[i].x == px && enemies[i].y == py) {
            lives--;
            if (lives <= 0) over = 1;
            else {
                px = W / 2; py = H / 2;
                // Clear around new spawn
                for (int dy = -1; dy <= 1; dy++)
                    for (int dx = -1; dx <= 1; dx++)
                        if (py + dy >= 0 && py + dy < H && px + dx >= 0 && px + dx < W)
                            grid[py + dy][px + dx] = 0;
            }
        }
    }
}

static void try_pump(void) {
    if (pump_timer > 0) return;
    // Check adjacent cells for an enemy
    int dirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
    for (int d = 0; d < 4; d++) {
        int ex = px + dirs[d][0];
        int ey = py + dirs[d][1];
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (!enemies[i].active) continue;
            if (enemies[i].x == ex && enemies[i].y == ey) {
                enemies[i].hp--;
                pump_dir = d;
                pump_timer = 8;
                if (enemies[i].hp <= 0) {
                    enemies[i].active = 0;
                    score += 20 * level;
                }
                return;
            }
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

    init_theme_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_theme_pair(2, COLOR_YELLOW, COLOR_BLACK);
    init_theme_pair(3, COLOR_RED, COLOR_BLACK);
    init_theme_pair(4, COLOR_WHITE, COLOR_BLACK);
    init_theme_pair(5, COLOR_CYAN, COLOR_BLACK);
    init_theme_pair(6, COLOR_MAGENTA, COLOR_BLACK);
    apply_theme_bg();

    int play_again = 1;
    while (play_again) {
        init_game();
        int ch;
        int oy = (LINES - H - 4) / 2;
        int ox = (COLS - W) / 2;

        while (1) {
            while (!over && !won) {
                ch = getch();
                if (ch == 'q') { over = 1; play_again = 0; break; }

                int nx = px, ny = py;
                if (ch == KEY_LEFT || ch == 'a') nx--;
                else if (ch == KEY_RIGHT || ch == 'd') nx++;
                else if (ch == KEY_UP || ch == 'w') ny--;
                else if (ch == KEY_DOWN || ch == 's') ny++;
                if (nx >= 0 && nx < W && ny >= 0 && ny < H) {
                    px = nx; py = ny;
                    grid[py][px] = 0; // dig
                }

                if (ch == ' ') try_pump();
                if (pump_timer > 0) pump_timer--;

                // Spawn enemies over time
                enemy_spawn_timer++;
                int spawn_rate = 80 - level * 10;
                if (spawn_rate < 30) spawn_rate = 30;
                if (enemy_spawn_timer >= spawn_rate) {
                    enemy_spawn_timer = 0;
                    spawn_enemy();
                }

                if (frame % (5 - level / 2) == 0)
                    move_enemies();

                // Check win: all enemies cleared
                int alive = 0;
                for (int i = 0; i < MAX_ENEMIES; i++)
                    if (enemies[i].active) alive++;
                if (alive == 0 && enemy_spawn_timer > 50) {
                    if (level < 3) { level++; won = 1; }
                    else won = 1;
                }

                erase();

                // Dirt and tunnels
                for (int y = 0; y < H; y++) {
                    for (int x = 0; x < W; x++) {
                        int ch = ' ';
                        if (grid[y][x]) {
                            ch = ((x + y) % 3 == 0) ? ':' : '.';
                            attron(COLOR_PAIR(4) | A_DIM);
                        } else {
                            attron(COLOR_PAIR(6));
                        }
                        mvaddch(oy + y, ox + x, ch);
                        attroff(COLOR_PAIR(4) | A_DIM);
                        attroff(COLOR_PAIR(6));
                    }
                }

                // Enemies
                for (int i = 0; i < MAX_ENEMIES; i++) {
                    if (!enemies[i].active) continue;
                    int color = (enemies[i].hp > PUMP_MAX / 2) ? 3 : 2;
                    attron(COLOR_PAIR(color) | A_BOLD);
                    mvaddch(oy + enemies[i].y, ox + enemies[i].x, '&');
                    attroff(COLOR_PAIR(color) | A_BOLD);
                }

                // Pump effect
                if (pump_timer > 4) {
                    int dirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
                    int pex = px + dirs[pump_dir][0];
                    int pey = py + dirs[pump_dir][1];
                    attron(COLOR_PAIR(2) | A_BOLD);
                    mvaddch(oy + pey, ox + pex, '*');
                    attroff(COLOR_PAIR(2) | A_BOLD);
                }

                // Player
                attron(COLOR_PAIR(2) | A_BOLD);
                mvaddch(oy + py, ox + px, '@');
                attroff(COLOR_PAIR(2) | A_BOLD);

                // HUD
                attron(COLOR_PAIR(5));
                char buf[64];
                snprintf(buf, sizeof(buf), _("Score: %d  Lives: %d  Level: %d", "Pontos: %d  Vidas: %d  Nivel: %d"),
                         score, lives, level);
                mvaddstr(oy + H + 1, ox, buf);
                mvaddstr(oy + H + 2, ox,
                         _("Arrows: move | Space: pump | Q: quit",
                           "Setas: mover | Espaco: bombear | Q: sair"));
                attroff(COLOR_PAIR(5));

                refresh();
                usleep(50000);
                frame++;
            }

            if (!play_again) break;

            if (won && level <= 3) {
                erase();
                char buf[64];
                attron(COLOR_PAIR(2) | A_BOLD);
                snprintf(buf, sizeof(buf), _("LEVEL %d CLEAR!", "NIVEL %d CONCLUIDO!"), level - 1);
                mvaddstr(oy + H / 2, ox + W / 2 - 8, buf);
                attroff(COLOR_PAIR(2) | A_BOLD);
                attron(COLOR_PAIR(5));
                mvaddstr(oy + H / 2 + 2, ox + W / 2 - 12,
                         _("Get ready...", "Prepare-se..."));
                attroff(COLOR_PAIR(5));
                refresh();
                nodelay(stdscr, FALSE);
                getch();
                nodelay(stdscr, TRUE);
                // Reset for next level
                for (int y = 0; y < H; y++)
                    for (int x = 0; x < W; x++)
                        grid[y][x] = 1;
                px = W / 2; py = H / 2;
                for (int dy = -1; dy <= 1; dy++)
                    for (int dx = -1; dx <= 1; dx++)
                        if (py + dy >= 0 && py + dy < H && px + dx >= 0 && px + dx < W)
                            grid[py + dy][px + dx] = 0;
                for (int i = 0; i < MAX_ENEMIES; i++) enemies[i].active = 0;
                enemy_spawn_timer = 0;
                won = 0;
                continue;
            }
            break;
        }

        if (play_again) {
            erase();
            char buf[64];
            if (won) {
                attron(COLOR_PAIR(2) | A_BOLD);
                mvaddstr(oy + H / 2, ox + W / 2 - 6, _("YOU WIN!", "VOCE VENCEU!"));
                attroff(COLOR_PAIR(2) | A_BOLD);
                attron(COLOR_PAIR(5));
                snprintf(buf, sizeof(buf), _("Final Score: %d", "Pontuacao Final: %d"), score);
                mvaddstr(oy + H / 2 + 2, ox + W / 2 - 8, buf);
            } else {
                attron(COLOR_PAIR(3) | A_BOLD);
                mvaddstr(oy + H / 2, ox + W / 2 - 6, _("GAME OVER", "FIM DE JOGO"));
                attroff(COLOR_PAIR(3) | A_BOLD);
                attron(COLOR_PAIR(5));
                snprintf(buf, sizeof(buf), _("Score: %d", "Pontuacao: %d"), score);
                mvaddstr(oy + H / 2 + 2, ox + W / 2 - 8, buf);
            }
            attron(COLOR_PAIR(5));
            mvaddstr(oy + H / 2 + 4, ox + W / 2 - 14,
                     _("Enter: play again | Q: quit", "Enter: jogar novamente | Q: sair"));
            attroff(COLOR_PAIR(5));
            refresh();
            nodelay(stdscr, FALSE);
            while (1) {
                ch = getch();
                if (ch == 'q') { play_again = 0; break; }
                if (ch == '\n') break;
            }
            nodelay(stdscr, TRUE);
        }
    }

    endwin();
    return 0;
}
