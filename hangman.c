/******************************************************************************
 *                              HANGMAN
 *           Guess the word before the man is hanged!
 ******************************************************************************/
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <locale.h>
#include "config.h"

#define MAX_WRONG 6
#define MAX_WORDS 25

static const char *words_en[MAX_WORDS] = {
    "apple", "banana", "castle", "dragon", "eagle",
    "forest", "garden", "hammer", "island", "jungle",
    "knight", "lemon", "mountain", "noble", "orange",
    "piano", "queen", "ruby", "silver", "thunder",
    "umbrella", "violet", "winter", "yellow", "zebra"
};
static const char *words_pt[MAX_WORDS] = {
    "abacaxi", "banana", "castelo", "dragao", "elefante",
    "floresta", "geladeira", "hipopotamo", "ilha", "janela",
    "labirinto", "macaco", "navio", "orelha", "piano",
    "queijo", "relogio", "sapato", "tesouro", "urso",
    "vassoura", "xadrez", "zebra", "arvore", "estrela"
};

static const char *hangman_gfx[MAX_WRONG + 1][7] = {
    {
        "  +---+  ",
        "  |   |  ",
        "      |  ",
        "      |  ",
        "      |  ",
        "      |  ",
        "========="
    },
    {
        "  +---+  ",
        "  |   |  ",
        "  O   |  ",
        "      |  ",
        "      |  ",
        "      |  ",
        "========="
    },
    {
        "  +---+  ",
        "  |   |  ",
        "  O   |  ",
        "  |   |  ",
        "      |  ",
        "      |  ",
        "========="
    },
    {
        "  +---+  ",
        "  |   |  ",
        "  O   |  ",
        " /|   |  ",
        "      |  ",
        "      |  ",
        "========="
    },
    {
        "  +---+  ",
        "  |   |  ",
        "  O   |  ",
        " /|\\  |  ",
        "      |  ",
        "      |  ",
        "========="
    },
    {
        "  +---+  ",
        "  |   |  ",
        "  O   |  ",
        " /|\\  |  ",
        " /    |  ",
        "      |  ",
        "========="
    },
    {
        "  +---+  ",
        "  |   |  ",
        "  O   |  ",
        " /|\\  |  ",
        " / \\  |  ",
        "      |  ",
        "========="
    }
};

static int wrong, guessed[26], word_len, game_over, won;
static char word[32], display[32];
static int wins, losses;

static void new_game(void) {
    const char **pool = (cfg_lang == LANG_PT) ? words_pt : words_en;
    int idx = rand() % MAX_WORDS;
    strcpy(word, pool[idx]);
    word_len = strlen(word);
    for (int i = 0; i < word_len; i++)
        word[i] = tolower(word[i]);
    memset(display, '_', word_len);
    display[word_len] = 0;
    memset(guessed, 0, sizeof(guessed));
    wrong = 0;
    game_over = won = 0;
}

static int all_guessed(void) {
    for (int i = 0; i < word_len; i++)
        if (!guessed[word[i] - 'a']) return 0;
    return 1;
}

static void draw(int ox, int oy) {
    erase();

    attron(COLOR_PAIR(2) | A_BOLD);
    mvaddstr(oy, ox + 12, _("HANGMAN", "JOGO DA FORCA"));
    attroff(COLOR_PAIR(2) | A_BOLD);

    /* Hangman graphic */
    for (int i = 0; i < 7; i++)
        mvaddstr(oy + 2 + i, ox, hangman_gfx[wrong][i]);

    /* Word display */
    attron(COLOR_PAIR(5) | A_BOLD);
    int wx = ox + 14;
    char buf[64];
    for (int i = 0; i < word_len; i++)
        buf[i] = display[i];
    buf[word_len] = 0;
    mvaddstr(oy + 4, wx, buf);
    attroff(COLOR_PAIR(5) | A_BOLD);

    /* Wrong guesses */
    attron(COLOR_PAIR(4));
    mvaddstr(oy + 6, wx, _("Wrong:", "Erros:"));
    attroff(COLOR_PAIR(4));
    int wc = 0;
    for (int i = 0; i < 26; i++) {
        if (guessed[i]) {
            int found = 0;
            for (int j = 0; j < word_len; j++)
                if (word[j] == 'a' + i) { found = 1; break; }
            if (!found) {
                char ch[2] = {'a' + i, 0};
                attron(COLOR_PAIR(4) | A_BOLD);
                mvaddstr(oy + 7, wx + wc * 2, ch);
                attroff(COLOR_PAIR(4) | A_BOLD);
                wc++;
            }
        }
    }

    /* Attempts remaining */
    attron(COLOR_PAIR(6));
    snprintf(buf, 64, _("Attempts left: %d", "Tentativas: %d"), MAX_WRONG - wrong);
    mvaddstr(oy + 9, wx, buf);
    attroff(COLOR_PAIR(6));

    /* Stats */
    snprintf(buf, 64, _("W: %d  L: %d", "V: %d  D: %d"), wins, losses);
    attron(COLOR_PAIR(5));
    mvaddstr(oy, wx, buf);
    attroff(COLOR_PAIR(5));

    /* Already guessed letters */
    attron(COLOR_PAIR(6) | A_DIM);
    mvaddstr(oy + 11, wx, _("Letters:", "Letras:"));
    int gc = 0;
    for (int i = 0; i < 26; i++) {
        if (guessed[i]) {
            char ch[2] = {'a' + i, 0};
            mvaddstr(oy + 12 + gc / 13, wx + (gc % 13) * 2, ch);
            gc++;
        }
    }
    attroff(COLOR_PAIR(6) | A_DIM);

    /* Input prompt */
    attron(COLOR_PAIR(2) | A_DIM);
    if (!game_over)
        mvaddstr(oy + 15, wx, _("Type a letter (a-z):", "Digite uma letra (a-z):"));
    attroff(COLOR_PAIR(2) | A_DIM);

    if (game_over) {
        attron(COLOR_PAIR(won ? 3 : 4) | A_BOLD);
        if (won) {
            snprintf(buf, 64, _("You won! The word was: %s", "Voce venceu! A palavra era: %s"), word);
            mvaddstr(oy + 13, wx, buf);
        } else {
            snprintf(buf, 64, _("Game over! The word was: %s", "Fim de jogo! A palavra era: %s"), word);
            mvaddstr(oy + 13, wx, buf);
        }
        attroff(COLOR_PAIR(won ? 3 : 4) | A_BOLD);

        attron(COLOR_PAIR(6) | A_DIM);
        mvaddstr(oy + 17, wx, _("Enter: play again  Q: quit", "Enter: jogar novamente  Q: sair"));
        attroff(COLOR_PAIR(6) | A_DIM);
    }

    attron(COLOR_PAIR(4) | A_DIM);
    mvaddstr(oy + 14, wx, _("Q: quit", "Q: sair"));
    attroff(COLOR_PAIR(4) | A_DIM);

    refresh();
}

int main(void) {
    setlocale(LC_ALL, "");
    srand(time(NULL));
    read_config();
    initscr(); cbreak(); noecho(); curs_set(0);
    keypad(stdscr, TRUE); nodelay(stdscr, FALSE);
    start_color();

    init_theme_pair(1, COLOR_CYAN, COLOR_BLACK);
    init_theme_pair(2, COLOR_YELLOW, COLOR_BLACK);
    init_theme_pair(3, COLOR_GREEN, COLOR_BLACK);
    init_theme_pair(4, COLOR_RED, COLOR_BLACK);
    init_theme_pair(5, COLOR_WHITE, COLOR_BLACK);
    init_theme_pair(6, COLOR_MAGENTA, COLOR_BLACK);
    apply_theme_bg();

    wins = losses = 0;
    new_game();

    int play = 1;
    while (play) {
        int ox = (COLS - 30) / 2 < 1 ? 1 : (COLS - 30) / 2;
        int oy = (LINES - 20) / 2 < 1 ? 1 : (LINES - 20) / 2;
        draw(ox, oy);

        int ch = getch();
        if (ch == 'q') break;

        if (game_over) {
            if (ch == '\n' || ch == ' ') { new_game(); }
            continue;
        }

        if (ch >= 'A' && ch <= 'Z') ch = tolower(ch);
        if (ch < 'a' || ch > 'z') continue;

        int idx = ch - 'a';
        if (guessed[idx]) continue;
        guessed[idx] = 1;

        int found = 0;
        for (int i = 0; i < word_len; i++) {
            if (word[i] == ch) {
                display[i] = ch;
                found = 1;
            }
        }
        if (!found) wrong++;

        if (all_guessed()) { game_over = 1; won = 1; wins++; }
        else if (wrong >= MAX_WRONG) { game_over = 1; won = 0; losses++; }
    }

    endwin();
    return 0;
}
