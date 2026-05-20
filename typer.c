/******************************************************************************
 *                           THE TYPER
 *         Type words to survive the zombie apocalypse!
 ******************************************************************************/
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <locale.h>
#include "config.h"

#define MAX_ZOMBIES 8
#define MAX_WORD 24
#define MAX_LIVES 3
#define WAVE_BASE 4
#define PLAY_ROW 22
#define SPAWN_INTERVAL 60

static const char *words_en[] = {
    "apple", "brain", "crawl", "death", "eaten",
    "flesh", "ghost", "horde", "killer", "limb",
    "moan", "night", "panic", "blood", "scream",
    "virus", "zombie", "shamble", "rotting", "brains",
    "casket", "dagger", "crypt", "graves", "hunter",
    "maggot", "plague", "wretch", "banana", "nuclear",
    NULL
};
static const char *words_pt[] = {
    "abrir", "bravo", "crise", "dreno", "ebano",
    "fenda", "grito", "horda", "inferno", "junta",
    "lutar", "morte", "nuvem", "pavor", "quase",
    "rugir", "sangue", "triste", "ursina", "viver",
    "zumbi", "caixao", "punhal", "morto", "tumba",
    "virus", "peste", "misera", "banana", "nuclear",
    NULL
};

typedef struct {
    char word[MAX_WORD];
    int len, typed;
    int row, col;
    int active, speed, counter;
} Zombie;

static Zombie zombies[MAX_ZOMBIES];
static int lives, score, wave, total_killed;
static int spawned, spawn_timer, game_over;
static int zombie_count;

static int frame;

static void new_wave(void) {
    wave++;
    zombie_count = WAVE_BASE + wave;
    if (zombie_count > MAX_ZOMBIES) zombie_count = MAX_ZOMBIES;
    spawned = 0;
    spawn_timer = 0;
    for (int i = 0; i < MAX_ZOMBIES; i++) zombies[i].active = 0;
}

static void spawn_zombie(void) {
    if (spawned >= zombie_count) return;
    for (int i = 0; i < MAX_ZOMBIES; i++) {
        if (!zombies[i].active) {
            const char **pool = (cfg_lang == LANG_PT) ? words_pt : words_en;
            int n = 0;
            while (pool[n]) n++;
            int idx;
            do { idx = rand() % n; } while ((int)strlen(pool[idx]) < 3 + wave / 3);
            strcpy(zombies[i].word, pool[idx]);
            zombies[i].len = strlen(zombies[i].word);
            zombies[i].typed = 0;
            zombies[i].row = 0;
            zombies[i].col = 2 + (rand() % 30);
            if (zombies[i].col + zombies[i].len > 38)
                zombies[i].col = 38 - zombies[i].len;
            zombies[i].active = 1;
            zombies[i].counter = 0;
            zombies[i].speed = 24 - wave;
            if (zombies[i].speed < 8) zombies[i].speed = 8;
            spawned++;
            return;
        }
    }
}

static void update_zombies(void) {
    for (int i = 0; i < MAX_ZOMBIES; i++) {
        if (!zombies[i].active) continue;
        zombies[i].counter++;
        if (zombies[i].counter >= zombies[i].speed) {
            zombies[i].counter = 0;
            zombies[i].row++;
            if (zombies[i].row >= PLAY_ROW) {
                zombies[i].active = 0;
                lives--;
                if (lives <= 0) game_over = 1;
            }
        }
    }
}

static int find_target(char ch) {
    int best = -1, best_row = -1;
    for (int i = 0; i < MAX_ZOMBIES; i++) {
        if (!zombies[i].active) continue;
        if (zombies[i].typed < zombies[i].len
            && zombies[i].word[zombies[i].typed] == ch) {
            if (zombies[i].row > best_row) {
                best_row = zombies[i].row;
                best = i;
            }
        }
    }
    return best;
}

static void handle_input(int ch) {
    if (ch >= 'A' && ch <= 'Z') ch = tolower(ch);
    if (ch < 'a' || ch > 'z') return;

    int idx = find_target(ch);
    if (idx < 0) return;

    zombies[idx].typed++;
    if (zombies[idx].typed >= zombies[idx].len) {
        zombies[idx].active = 0;
        score += zombies[idx].len * 10 + wave * 5;
        total_killed++;
        if (total_killed >= zombie_count) {
            if (wave >= 10) game_over = 1;
        }
    }
}

static void draw_zombie(int ox, int oy, Zombie *z) {
    int y = oy + 3 + z->row;
    int x = ox + z->col;

    /* Zombie face */
    attron(COLOR_PAIR(4) | A_BOLD);
    mvaddstr(y, x, _("Z ", "Z "));
    attroff(COLOR_PAIR(4) | A_BOLD);

    /* Word display */
    x += 2;
    for (int i = 0; i < z->len; i++) {
        if (i < z->typed) {
            attron(COLOR_PAIR(3) | A_BOLD);
            mvaddch(y, x + i, z->word[i]);
            attroff(COLOR_PAIR(3) | A_BOLD);
        } else {
            attron(COLOR_PAIR(7));
            mvaddch(y, x + i, z->word[i]);
            attroff(COLOR_PAIR(7));
        }
    }
}

static void draw_stats(int ox, int oy) {
    char buf[64];
    attron(COLOR_PAIR(5) | A_BOLD);
    mvaddstr(oy, ox, _("THE TYPER", "THE TYPER"));
    attroff(COLOR_PAIR(5) | A_BOLD);

    snprintf(buf, 64, _("Wave: %d  Score: %d  Lives: ", "Onda: %d  Pontos: %d  Vidas: "), wave, score);
    attron(COLOR_PAIR(2));
    mvaddstr(oy + 1, ox, buf);
    attroff(COLOR_PAIR(2));
    for (int i = 0; i < MAX_LIVES; i++) {
        attron(i < lives ? COLOR_PAIR(4) : COLOR_PAIR(6) | A_DIM);
        mvaddch(oy + 1, ox + strlen(buf) + i, '<' + 3);
        attroff(i < lives ? COLOR_PAIR(4) : COLOR_PAIR(6) | A_DIM);
    }

    attron(COLOR_PAIR(4) | A_DIM);
    mvaddstr(oy + 23, ox,
             _("Type words to kill zombies  Q: quit",
               "Digite palavras para matar zumbis  Q: sair"));
    attroff(COLOR_PAIR(4) | A_DIM);
}

static void draw_game_over(int ox, int oy) {
    attron(COLOR_PAIR(4) | A_BOLD);
    mvaddstr(oy + 10, ox + 6, _("GAME OVER", "FIM DE JOGO"));
    attroff(COLOR_PAIR(4) | A_BOLD);

    char buf[64];
    snprintf(buf, 64, _("Wave reached: %d   Final score: %d", "Onda: %d   Pontuacao: %d"), wave, score);
    attron(COLOR_PAIR(2));
    mvaddstr(oy + 12, ox + 6, buf);
    attroff(COLOR_PAIR(2));

    if (wave >= 10) {
        attron(COLOR_PAIR(3) | A_BOLD);
        mvaddstr(oy + 11, ox + 6, _("You survived!", "Voce sobreviveu!"));
        attroff(COLOR_PAIR(3) | A_BOLD);
    }

    attron(COLOR_PAIR(6) | A_DIM);
    mvaddstr(oy + 14, ox + 6, _("Enter: play again  Q: quit", "Enter: jogar novamente  Q: sair"));
    attroff(COLOR_PAIR(6) | A_DIM);
}

static void draw(int ox, int oy) {
    erase();
    draw_stats(ox, oy);

    int h = PLAY_ROW;
    for (int r = 0; r < h; r++) {
        attron(COLOR_PAIR(6) | A_DIM);
        mvaddch(oy + 3 + r, ox + 39, '|');
        attroff(COLOR_PAIR(6) | A_DIM);
    }

    /* Ground line */
    attron(COLOR_PAIR(4) | A_DIM);
    for (int c = 0; c < 41; c++)
        mvaddch(oy + 3 + PLAY_ROW, ox + c, '=');
    attroff(COLOR_PAIR(4) | A_DIM);

    /* Attack indicator */
    attron(COLOR_PAIR(4) | A_BOLD);
    mvaddstr(oy + 3 + PLAY_ROW + 1, ox + 2,
             _("YOU", "VOCE"));
    attroff(COLOR_PAIR(4) | A_BOLD);

    for (int i = 0; i < MAX_ZOMBIES; i++)
        if (zombies[i].active)
            draw_zombie(ox, oy, &zombies[i]);

    if (game_over) draw_game_over(ox, oy);

    refresh();
}

static void new_game(void) {
    lives = MAX_LIVES;
    score = 0;
    wave = 0;
    total_killed = 0;
    game_over = 0;
    frame = 0;
    new_wave();
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
    init_theme_pair(7, COLOR_WHITE, COLOR_BLACK);
    apply_theme_bg();

    int play = 1;
    while (play) {
        new_game();

        while (1) {
            int ox = (COLS - 42) / 2 < 1 ? 1 : (COLS - 42) / 2;
            int oy = (LINES - 26) / 2 < 1 ? 1 : (LINES - 26) / 2;
            draw(ox, oy);

            int ch = getch();
            if (ch == 'q') { play = 0; break; }

            if (game_over) {
                if (ch == '\n' || ch == ' ') break;
                usleep(16000);
                continue;
            }

            if (ch != ERR) handle_input(ch);

            /* Spawn logic */
            if (spawned < zombie_count) {
                spawn_timer++;
                if (spawn_timer >= SPAWN_INTERVAL) {
                    spawn_timer = 0;
                    spawn_zombie();
                }
            }

            update_zombies();

            /* Check wave complete */
            if (spawned >= zombie_count && total_killed >= zombie_count) {
                if (wave >= 10) {
                    game_over = 1;
                } else {
                    new_wave();
                }
            }

            usleep(33000);
        }
        if (!play) break;
    }

    endwin();
    return 0;
}
