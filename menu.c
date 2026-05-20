/******************************************************************************
 *                           SHELL GAMES
 *               Terminal Arcade Collection - Main Menu
 ******************************************************************************/
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "config.h"

#define TOTAL 26

static const char **games;
static const char **paths;
static const char **desc;
static const char *game_names_en[TOTAL] = {
    "1. 2048", "2. Asteroids", "3. Blackjack", "4. Breakout",
    "5. Centipede", "6. Checkers", "7. Chess", "8. Dig Dug",
    "9. Donkey Kong", "10. Duck Hunt", "11. Enduro",
    "12. Frogger", "13. Galaga", "14. Hangman",
    "15. Minesweeper", "16. Pac-Man", "17. Pong",
    "18. Q*Bert", "19. Rhythm Spire", "20. Snake",
    "21. Solitaire", "22. Spire Ascent", "23. Super Auto Pets",
    "24. Tetris", "25. The Typer", "26. Settings"
};
static const char *game_names_pt[TOTAL] = {
    "1. 2048", "2. Asteroides", "3. Blackjack", "4. Breakout",
    "5. Centipede", "6. Damas", "7. Xadrez", "8. Dig Dug",
    "9. Donkey Kong", "10. Duck Hunt", "11. Enduro",
    "12. Frogger", "13. Galaga", "14. Jogo da Forca",
    "15. Campo Minado", "16. Pac-Man", "17. Pong",
    "18. Q*Bert", "19. Ritmo na Spire", "20. Snake",
    "21. Paciencia", "22. Spire Ascent", "23. Super Auto Pets",
    "24. Tetris", "25. The Typer", "26. Configuracoes"
};
static const char *paths_list[TOTAL] = {
    "game2048", "asteroids", "blackjack", "breakout",
    "centipede", "checkers", "chess", "digdug",
    "donkeykong", "duckhunt", "enduro",
    "frogger", "galaga", "hangman",
    "minesweeper", "pacman", "pong",
    "qbert", "rhythm", "snake",
    "paciencia", "spire", "superautopets",
    "tetris", "typer", ""
};
static const char *desc_en[TOTAL] = {
    "  Slide and merge tiles to reach 2048!",
    "  Blast asteroids in deep space!",
    "  Beat the dealer in classic 21!",
    "  Break all the bricks with paddle and ball!",
    "  Shoot the creeping centipede!",
    "  Classic draughts for two players",
    "  Classic chess for two players",
    "  Dig tunnels and pump enemies!",
    "  Climb platforms dodging barrels",
    "  Aim and shoot ducks in this classic gallery!",
    "  Race through traffic across 4 days!",
    "  Cross the highway to reach home!",
    "  Classic arcade shooter - defeat alien waves!",
    "  Guess the word before the man is hanged!",
    "  Find all mines without exploding",
    "  Eat dots and avoid ghosts in the maze",
    "  Classic table tennis - 1 or 2 players!",
    "  Hop on cubes to change their color!",
    "  4-lane rhythm game - feel the beat!",
    "  Classic snake game - eat food and grow!",
    "  Classic Klondike solitaire card game",
    "  A deck-building roguelike - climb the Spire!",
    "  Auto-battler - collect pets and fight!",
    "  Stack falling blocks to clear lines",
    "  Type words to survive the zombie apocalypse!",
    "  Change language, theme and preferences"
};
static const char *desc_pt[TOTAL] = {
    "  Deslize e junte pecas ate 2048!",
    "  Exploda asteroides no espaco!",
    "  Venca a mesa no classico 21!",
    "  Quebre todos os tijolos com a raquete!",
    "  Atire na centopeia rastejante!",
    "  Damas classico para dois jogadores",
    "  Xadrez classico para dois jogadores",
    "  Cave tuneis e bombeie inimigos!",
    "  Suba plataformas desviando de barris",
    "  Mire e atire nos patos neste classico galeria!",
    "  Corra pelo transito por 4 dias!",
    "  Atravesse a rodovia para chegar em casa!",
    "  Tiro ao alien classico - derrote as ondas!",
    "  Adivinhe a palavra antes de ser enforcado!",
    "  Encontre todas as minas sem explodir",
    "  Coma pontos e evite fantasmas no labirinto",
    "  Tenis de mesa classico - 1 ou 2 jogadores!",
    "  Pule nos cubos para mudar a cor!",
    "  Jogo de ritmo 4-pistas - sinta o beat!",
    "  Snake classico - coma e cresca!",
    "  Paciencia clasico com cartas",
    "  Roguelike de cartas - conquiste a Spire!",
    "  Auto-batalha - colete pets e lute!",
    "  Empilhe blocos e limpe linhas",
    "  Digite palavras para sobreviver ao apocalipse zumbi!",
    "  Alterar idioma, tema e preferencias"
};

static void refresh_lang(void) {
    games = (cfg_lang == LANG_PT) ? game_names_pt : game_names_en;
    desc = (cfg_lang == LANG_PT) ? desc_pt : desc_en;
    paths = paths_list;
}

static void draw_title(void) {
    const char *title[] = {
        "      .---. .----.      ",
        "     /  ___/ |    |     ",
        "     | (___  |    |     ",
        "     \\___ \\ |    |     ",
        "     ____) ||    |      ",
        "    '____/ '----'       ",
        "                        ",
    };
    const char *sub = "    ~~ Terminal Arcade Collection ~~";
    int h = sizeof(title) / sizeof(title[0]);
    int y = 1;
    int w = (int)strlen(title[0]);
    for (int i = 0; i < h; i++)
        mvaddstr(y + i, (COLS - w) / 2, title[i]);
    mvaddstr(y + h, (COLS - (int)strlen(sub)) / 2, sub);
}

static void set_colors(void);
static void launch_game(int idx) {
    const char *name = paths[idx];
    char cmd[256];
    snprintf(cmd, sizeof cmd,
        "if [ -x ./%s ]; then ./%s; "
        "elif command -v shellgame-%s >/dev/null 2>&1; then shellgame-%s; "
        "else %s; fi",
        name, name, name, name, name);
    endwin();
    system(cmd);
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    start_color();
    set_colors();
}

static void set_colors(void) {
    init_theme_pair(1, COLOR_CYAN, COLOR_BLACK);
    init_theme_pair(2, COLOR_YELLOW, COLOR_BLACK);
    init_theme_pair(3, COLOR_GREEN, COLOR_BLACK);
    init_theme_pair(4, COLOR_RED, COLOR_BLACK);
    init_theme_pair(5, COLOR_MAGENTA, COLOR_BLACK);
    init_theme_pair(6, COLOR_BLUE, COLOR_BLACK);
    init_theme_pair(7, COLOR_WHITE, COLOR_BLACK);
    apply_theme_bg();
}

static void settings_menu(void) {
    int sel = 0, ch, exit_settings = 0;
    int lang_opts = 0, theme_opts = 0;

    while (!exit_settings) {
        erase();
        int my = 3, mx = COLS / 2 - 20;

        attron(COLOR_PAIR(1) | A_BOLD);
        mvaddstr(my, mx + 8, _("SETTINGS", "CONFIGURACOES"));
        attroff(COLOR_PAIR(1) | A_BOLD);

        my += 2;
        mvaddstr(my, mx, _("Language / Idioma:", "Idioma / Language:"));
        my++;
        const char *langs[] = {"English", "Portugues"};
        for (int i = 0; i < 2; i++) {
            if (lang_opts == i && sel == 0) attron(A_REVERSE);
            mvaddstr(my, mx + 4, langs[i]);
            if (cfg_lang == (i == 0 ? LANG_EN : LANG_PT)) mvaddstr(my, mx + 16, _(" (current)", " (atual)"));
            if (lang_opts == i && sel == 0) attroff(A_REVERSE);
            my++;
        }

        my++;
        mvaddstr(my, mx, _("Theme:", "Tema:"));
        my++;
        const char *themes[] = {_("Dark", "Escuro"), _("Light", "Claro"), _("Retro", "Retro")};
        for (int i = 0; i < 3; i++) {
            if (theme_opts == i && sel == 1) attron(A_REVERSE);
            mvaddstr(my, mx + 4, themes[i]);
            if (cfg_theme == (i == 0 ? THEME_DARK : i == 1 ? THEME_LIGHT : THEME_RETRO))
                mvaddstr(my, mx + 16, _(" (current)", " (atual)"));
            if (theme_opts == i && sel == 1) attroff(A_REVERSE);
            my++;
        }

        my += 2;
        attron(COLOR_PAIR(2) | A_DIM);
        mvaddstr(my, mx, _("UP/DOWN: navigate | LEFT/RIGHT: change | ESC: back",
                           "UP/DOWN: navegar | LEFT/RIGHT: alterar | ESC: voltar"));
        attroff(COLOR_PAIR(2) | A_DIM);

        refresh();
        ch = getch();
        if (ch == 27) { write_config(); exit_settings = 1; }
        if (ch == KEY_UP && sel > 0) sel--;
        if (ch == KEY_DOWN && sel < 1) sel++;
        if (ch == KEY_LEFT || ch == KEY_RIGHT) {
            if (sel == 0) {
                if (ch == KEY_RIGHT && lang_opts < 1) lang_opts++;
                if (ch == KEY_LEFT && lang_opts > 0) lang_opts--;
            }
            if (sel == 1) {
                if (ch == KEY_RIGHT && theme_opts < 2) theme_opts++;
                if (ch == KEY_LEFT && theme_opts > 0) theme_opts--;
            }
            cfg_lang = (lang_opts == 0) ? LANG_EN : LANG_PT;
            cfg_theme = (theme_opts == 0) ? THEME_DARK : (theme_opts == 1) ? THEME_LIGHT : THEME_RETRO;
            refresh_lang();
            // Re-init colors
            erase();
            start_color();
            set_colors();
        }
    }
}

int main(void) {
    read_config();
    refresh_lang();

    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    start_color();
    set_colors();

    int sel = 0, ch;
    while (1) {
        erase();

        attron(COLOR_PAIR(1));
        draw_title();
        attroff(COLOR_PAIR(1));

        int my = 11;
        int x = COLS / 2 - 25;
        for (int i = 0; i < TOTAL - 1; i++) {
            if (i == sel) {
                attron(A_REVERSE);
                mvaddstr(my + i, x, "  ");
                mvaddstr(my + i, x + 2, games[i]);
                mvaddstr(my + i, x + 20, desc[i]);
                attroff(A_REVERSE);
            } else {
                mvaddstr(my + i, x + 2, games[i]);
                mvaddstr(my + i, x + 20, desc[i]);
            }
        }
        // Separator line
        attron(COLOR_PAIR(5) | A_DIM);
        mvaddstr(my + TOTAL - 1, x + 2, "--------------------------");
        attroff(COLOR_PAIR(5) | A_DIM);
        // Settings item
        int si = my + TOTAL;
        if (sel == TOTAL - 1) {
            attron(A_REVERSE);
            mvaddstr(si, x, "  ");
            mvaddstr(si, x + 2, games[TOTAL - 1]);
            mvaddstr(si, x + 20, desc[TOTAL - 1]);
            attroff(A_REVERSE);
        } else {
            attron(COLOR_PAIR(5) | A_DIM);
            mvaddstr(si, x + 2, games[TOTAL - 1]);
            mvaddstr(si, x + 20, desc[TOTAL - 1]);
            attroff(COLOR_PAIR(5) | A_DIM);
        }

        attron(COLOR_PAIR(2) | A_DIM);
        mvaddstr(si + 2, COLS / 2 - 18,
                 _("Arrows: navigate | Enter: play | Q: quit",
                   "Setas: navegar | Enter: jogar | Q: sair"));
        attroff(COLOR_PAIR(2) | A_DIM);

        refresh();
        ch = getch();
        if (ch == 'q') break;
        if (ch == KEY_UP && sel > 0) sel--;
        if (ch == KEY_DOWN && sel < TOTAL - 1) sel++;

        if (ch == '\n' || ch == ' ') {
            if (sel == TOTAL - 1) {
                // Settings
                settings_menu();
                // Re-init after settings return
                start_color();
                set_colors();
            } else {
                launch_game(sel);
            }
        }
    }

    erase();
    attron(COLOR_PAIR(3) | A_BOLD);
    mvaddstr(LINES / 2, COLS / 2 - 10,
             _("See you later!", "Ate logo!"));
    attroff(COLOR_PAIR(3) | A_BOLD);
    refresh();
    usleep(1000000);
    endwin();
    return 0;
}
