/******************************************************************************
 *                                FROGGER
 *         Guide the frog across a busy highway to reach home!
 ******************************************************************************/
#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include "config.h"

#define W 27
#define H 17
#define LANE_COUNT 9
#define MAX_CARS 30
#define GOAL_SPOTS 5
#define FROG_SPEED 2

typedef struct { int y, x, w, dir, active; } Car;

static int frog_x, frog_y;
static Car cars[MAX_CARS];
static int goals[GOAL_SPOTS];
static int score, lives, level, over, won;
static int frame, move_timer;

static const int lane_y[LANE_COUNT] = { 3,4,5,6,7,8,9,10,11 };
static const int lane_dir[LANE_COUNT] = { 1,-1,1,-1,1,-1,1,-1,1 };
static const int lane_density[LANE_COUNT] = { 30,25,35,22,30,25,35,22,30 };

static void init_game(void) {
    frog_x = W / 2;
    frog_y = H - 2;
    score = 0; lives = 3; level = 1; over = 0; won = 0;
    frame = 0; move_timer = 0;
    for (int i = 0; i < GOAL_SPOTS; i++) goals[i] = 0;
    for (int i = 0; i < MAX_CARS; i++) cars[i].active = 0;
}

static void spawn_car(int li) {
    for (int i = 0; i < MAX_CARS; i++)
        if (!cars[i].active) {
            int w = (rand() % 3 == 0) ? 4 : 3;
            int x = (lane_dir[li] == 1) ? -w : W;
            cars[i] = (Car){ lane_y[li], x, w, lane_dir[li], 1 };
            break;
        }
}

static void update_cars(void) {
    for (int i = 0; i < MAX_CARS; i++) {
        if (!cars[i].active) continue;
        cars[i].x += cars[i].dir;
        if (cars[i].x > W + 2 || cars[i].x < -cars[i].w - 2)
            cars[i].active = 0;
    }
}

static int car_at(int y, int x) {
    for (int i = 0; i < MAX_CARS; i++)
        if (cars[i].active && cars[i].y == y &&
            x >= cars[i].x && x < cars[i].x + cars[i].w)
            return 1;
    return 0;
}

static void check_collision(void) {
    for (int li = 0; li < LANE_COUNT; li++)
        if (frog_y == lane_y[li]) {
            if (car_at(frog_y, frog_x)) {
                lives--;
                if (lives <= 0) over = 1;
                else { frog_x = W / 2; frog_y = H - 2; }
                return;
            }
        }
}

static void check_goal(void) {
    if (frog_y == 1 || frog_y == 0) {
        // Map frog_x to a goal spot (0 to GOAL_SPOTS-1)
        int spot = (frog_x - 2) / 4;
        if (spot >= 0 && spot < GOAL_SPOTS && !goals[spot]) {
            goals[spot] = 1;
            score += 10 * level;
            frog_x = W / 2;
            frog_y = H - 2;
            // Check all goals filled
            int all = 1;
            for (int i = 0; i < GOAL_SPOTS; i++)
                if (!goals[i]) all = 0;
            if (all) {
                if (level < 3) { level++; won = 1; }
                else { won = 1; }
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
    init_theme_pair(6, COLOR_BLUE, COLOR_BLACK);
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

                if (move_timer == 0) {
                    if ((ch == KEY_UP || ch == 'w') && frog_y > 0) frog_y--;
                    if ((ch == KEY_DOWN || ch == 's') && frog_y < H - 2) frog_y++;
                    if ((ch == KEY_LEFT || ch == 'a') && frog_x > 0) frog_x--;
                    if ((ch == KEY_RIGHT || ch == 'd') && frog_x < W - 2) frog_x++;
                    move_timer = FROG_SPEED;
                }
                if (move_timer > 0) move_timer--;

                if (frame % 3 == 0) {
                    for (int li = 0; li < LANE_COUNT; li++)
                        if (rand() % lane_density[li] == 0)
                            spawn_car(li);
                    update_cars();
                }

                check_collision();
                check_goal();

                erase();

                // Border
                attron(COLOR_PAIR(4));
                for (int x = 0; x < W; x++) {
                    mvaddch(oy, ox + x, '#');
                    mvaddch(oy + H + 1, ox + x, '#');
                }
                for (int y = 0; y < H + 2; y++) {
                    mvaddch(oy + y, ox, '#');
                    mvaddch(oy + y, ox + W - 1, '#');
                }
                attroff(COLOR_PAIR(4));

                // Safe zone (goal area) - top
                attron(COLOR_PAIR(1) | A_BOLD);
                for (int s = 0; s < GOAL_SPOTS; s++) {
                    int gx = ox + 2 + s * 4;
                    if (goals[s]) mvaddstr(oy + 1, gx, "*.*");
                    else          mvaddstr(oy + 1, gx, "[ ]");
                }
                attroff(COLOR_PAIR(1) | A_BOLD);

                // Grass shoulders
                attron(COLOR_PAIR(1));
                for (int x = 1; x < W - 1; x++) {
                    mvaddch(oy + 2, ox + x, '\'');
                    mvaddch(oy + 12, ox + x, '\'');
                }
                attroff(COLOR_PAIR(1));

                // Traffic lanes
                for (int li = 0; li < LANE_COUNT; li++) {
                    int ly = oy + 1 + lane_y[li];
                    attron(COLOR_PAIR(6));
                    for (int x = 1; x < W - 1; x++)
                        mvaddch(ly, ox + x, ' ');
                    attroff(COLOR_PAIR(6));
                    attron(COLOR_PAIR(5) | A_DIM);
                    if (lane_dir[li] == 1) {
                        for (int x = 1; x < W - 1; x += 4)
                            mvaddch(ly, ox + x, '.');
                    } else {
                        for (int x = 3; x < W - 1; x += 4)
                            mvaddch(ly, ox + x, '.');
                    }
                    attroff(COLOR_PAIR(5) | A_DIM);
                }

                // Cars
                attron(COLOR_PAIR(3) | A_BOLD);
                for (int i = 0; i < MAX_CARS; i++) {
                    if (!cars[i].active) continue;
                    int cy = oy + 1 + cars[i].y;
                    int cx = ox + cars[i].x;
                    if (cars[i].w == 4) {
                        mvaddch(cy, cx, '[');
                        mvaddch(cy, cx + 1, '#');
                        mvaddch(cy, cx + 2, '#');
                        mvaddch(cy, cx + 3, ']');
                    } else {
                        mvaddch(cy, cx, '[');
                        mvaddch(cy, cx + 1, '#');
                        mvaddch(cy, cx + 2, ']');
                    }
                }
                attroff(COLOR_PAIR(3) | A_BOLD);

                // Frog (blinking)
                if ((frame / 6) % 2 == 0) {
                    attron(COLOR_PAIR(2) | A_BOLD);
                    mvaddstr(oy + 1 + frog_y, ox + frog_x, "@");
                    attroff(COLOR_PAIR(2) | A_BOLD);
                }

                // HUD
                attron(COLOR_PAIR(5));
                char buf[64];
                snprintf(buf, sizeof(buf), _("Score: %d  Lives: %d  Level: %d", "Pontos: %d  Vidas: %d  Nivel: %d"),
                         score, lives, level);
                mvaddstr(oy + H + 3, ox, buf);
                mvaddstr(oy + H + 4, ox,
                         _("Arrows/WASD: move | Q: quit", "Setas/WASD: mover | Q: sair"));
                attroff(COLOR_PAIR(5));

                refresh();
                usleep(50000);
                frame++;
            }

            if (!play_again) break;

            if (won && level < 3) {
                erase();
                attron(COLOR_PAIR(2) | A_BOLD);
                char buf[64];
                snprintf(buf, sizeof(buf), _("LEVEL %d CLEAR!", "NIVEL %d CONCLUIDO!"), level);
                mvaddstr(oy + H / 2, ox + W/2 - 6, buf);
                attroff(COLOR_PAIR(2) | A_BOLD);
                attron(COLOR_PAIR(5));
                mvaddstr(oy + H / 2 + 2, ox + W/2 - 10,
                         _("Get ready...", "Prepare-se..."));
                attroff(COLOR_PAIR(5));
                refresh();
                nodelay(stdscr, FALSE);
                getch();
                nodelay(stdscr, TRUE);
                level++;
                for (int i = 0; i < GOAL_SPOTS; i++) goals[i] = 0;
                frog_x = W / 2; frog_y = H - 2;
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
                mvaddstr(oy + H / 2, ox + 6, _("YOU WIN!", "VOCE VENCEU!"));
                attroff(COLOR_PAIR(2) | A_BOLD);
                attron(COLOR_PAIR(5));
                snprintf(buf, sizeof(buf), _("Final Score: %d", "Pontuacao Final: %d"), score);
                mvaddstr(oy + H / 2 + 2, ox + 6, buf);
            } else {
                attron(COLOR_PAIR(3) | A_BOLD);
                mvaddstr(oy + H / 2, ox + 6, _("GAME OVER", "FIM DE JOGO"));
                attroff(COLOR_PAIR(3) | A_BOLD);
                attron(COLOR_PAIR(5));
                snprintf(buf, sizeof(buf), _("Score: %d", "Pontuacao: %d"), score);
                mvaddstr(oy + H / 2 + 2, ox + 6, buf);
            }
            attron(COLOR_PAIR(5));
            mvaddstr(oy + H / 2 + 4, ox + 2,
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
