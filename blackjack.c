/******************************************************************************
 *                           BLACKJACK
 *        Classic 21 — beat the dealer in the terminal!
 ******************************************************************************/
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <locale.h>
#include "config.h"

/* ───────── Card constants ───────── */
#define DECKS 6
#define MAX_HAND 12

static const char *suit_sym[4] = {
    "\xe2\x99\xa0", "\xe2\x99\xa5", "\xe2\x99\xa6", "\xe2\x99\xa3"
};
static const char *rank_str[14] = {
    "", "A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K"
};

/* ───────── Deck & Hands ───────── */
typedef struct { int rank, suit; } Card;

static Card shoe[DECKS * 52];
static int shoe_sz, shoe_pos;

typedef struct { Card cards[MAX_HAND]; int n; } Hand;
static Hand player, dealer;

static int wins, losses, pushes, round_num;
static int game_over, result; /* 0=playing, 1=player_win, 2=dealer_win, 3=push */
static int player_stand;

/* ───────── Shoe management ───────── */
static void build_shoe(void) {
    shoe_sz = DECKS * 52;
    for (int d = 0; d < DECKS; d++)
        for (int i = 0; i < 52; i++) {
            shoe[d * 52 + i].rank = (i % 13) + 1;
            shoe[d * 52 + i].suit = i / 13;
        }
    /* Fisher-Yates shuffle */
    for (int i = shoe_sz - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        Card t = shoe[i]; shoe[i] = shoe[j]; shoe[j] = t;
    }
    shoe_pos = 0;
}

static Card deal(void) {
    if (shoe_pos >= shoe_sz) build_shoe();
    return shoe[shoe_pos++];
}

static void deal_to(Hand *h) {
    if (h->n < MAX_HAND)
        h->cards[h->n++] = deal();
}

/* ───────── Hand value ───────── */
static int hand_val(Hand *h) {
    int total = 0, aces = 0;
    for (int i = 0; i < h->n; i++) {
        int r = h->cards[i].rank;
        if (r == 1) { total += 11; aces++; }
        else if (r > 10) total += 10;
        else total += r;
    }
    while (total > 21 && aces > 0) { total -= 10; aces--; }
    return total;
}

static int is_blackjack(Hand *h) {
    return h->n == 2 && hand_val(h) == 21;
}

/* ───────── Card display ───────── */
static void draw_card(int y, int x, Card *c, int hidden) {
    if (hidden) {
        attron(COLOR_PAIR(3) | A_BOLD);
        mvaddstr(y, x, "[??]  ");
        attroff(COLOR_PAIR(3) | A_BOLD);
        return;
    }
    int col = (c->suit == 1 || c->suit == 2) ? 3 : 4;
    attron(COLOR_PAIR(col) | A_BOLD);
    char buf[8];
    snprintf(buf, 7, "[%s%s]", rank_str[c->rank], suit_sym[c->suit]);
    /* pad to 6 chars for alignment */
    int len = (int)strlen(buf);
    while (len < 6) { buf[len] = ' '; len++; }
    buf[6] = '\0';
    mvaddstr(y, x, buf);
    attroff(COLOR_PAIR(col) | A_BOLD);
}

static void draw_hand(int y, int x, Hand *h, int hide_first, char *label) {
    attron(COLOR_PAIR(2) | A_BOLD);
    mvaddstr(y, x, label);
    attroff(COLOR_PAIR(2) | A_BOLD);

    for (int i = 0; i < h->n; i++)
        draw_card(y + 1, x + i * 5, &h->cards[i], hide_first && i == 0);

    char val[16];
    if (hide_first) {
        snprintf(val, 16, _("= ?", "= ?"));
    } else {
        snprintf(val, 16, "= %d", hand_val(h));
    }
    attron(COLOR_PAIR(5));
    mvaddstr(y + 1, x + h->n * 5 + 1, val);
    attroff(COLOR_PAIR(5));
}

/* ───────── Game init ───────── */
static void init_round(void) {
    player.n = 0; dealer.n = 0;
    player_stand = 0;
    game_over = 0; result = 0;
    round_num++;

    deal_to(&player);
    deal_to(&dealer);
    deal_to(&player);
    deal_to(&dealer);

    /* Check blackjack */
    if (is_blackjack(&player) || is_blackjack(&dealer)) {
        game_over = 1;
        if (is_blackjack(&player) && is_blackjack(&dealer)) result = 3;
        else if (is_blackjack(&player)) result = 1;
        else result = 2;
        if (result == 1) wins++;
        else if (result == 2) losses++;
        else pushes++;
    }
}

/* ───────── Dealer play ───────── */
static void dealer_play(void) {
    while (hand_val(&dealer) < 17)
        deal_to(&dealer);
}

/* ───────── Resolve ───────── */
static void resolve(void) {
    if (game_over) return;
    dealer_play();
    int pv = hand_val(&player);
    int dv = hand_val(&dealer);

    if (dv > 21) result = 1;
    else if (pv > dv) result = 1;
    else if (pv < dv) result = 2;
    else result = 3;

    if (result == 1) wins++;
    else if (result == 2) losses++;
    else pushes++;
    game_over = 1;
}

/* ───────── Draw screen ───────── */
static int screen_ox, screen_oy;

static void draw_game(void) {
    erase();
    int cx = screen_ox + 20;

    attron(COLOR_PAIR(2) | A_BOLD);
    mvaddstr(screen_oy, cx - 5, _("BLACKJACK", "BLACKJACK"));
    attroff(COLOR_PAIR(2) | A_BOLD);

    char rbuf[24];
    snprintf(rbuf, 24, _("Round %d", "Rodada %d"), round_num);
    attron(COLOR_PAIR(6) | A_DIM);
    mvaddstr(screen_oy, screen_ox + 30, rbuf);
    attroff(COLOR_PAIR(6) | A_DIM);

    /* Dealer hand */
    draw_hand(screen_oy + 2, screen_ox + 4, &dealer,
              !game_over && result == 0,
              _("Dealer:", "Mesa:"));

    /* Player hand */
    draw_hand(screen_oy + 8, screen_ox + 4, &player, 0,
              _("You:", "Voce:"));

    /* Result */
    if (game_over) {
        int ry = screen_oy + 5;
        attron(COLOR_PAIR(2) | A_BOLD);
        if (result == 1) {
            if (is_blackjack(&player))
                mvaddstr(ry, cx - 5, _("BLACKJACK!", "BLACKJACK!"));
            else
                mvaddstr(ry, cx - 4, _("YOU WIN!", "VOCE VENCEU!"));
        } else if (result == 2) {
            attron(COLOR_PAIR(3));
            mvaddstr(ry, cx - 5, _("DEALER WINS", "MESA VENCEU"));
            attroff(COLOR_PAIR(3));
        } else {
            attron(COLOR_PAIR(5));
            mvaddstr(ry, cx - 2, _("PUSH", "EMPATE"));
            attroff(COLOR_PAIR(5));
        }
        attroff(COLOR_PAIR(2) | A_BOLD);

        attron(COLOR_PAIR(6) | A_DIM);
        mvaddstr(screen_oy + 14, screen_ox + 4,
                 _("Enter: next round  Q: quit",
                   "Enter: proxima rodada  Q: sair"));
        attroff(COLOR_PAIR(6) | A_DIM);
    }

    /* Stats */
    char sbuf[48];
    snprintf(sbuf, 48, _("W: %d  L: %d  P: %d", "V: %d  D: %d  E: %d"),
             wins, losses, pushes);
    attron(COLOR_PAIR(5));
    mvaddstr(screen_oy + 16, screen_ox + 12, sbuf);
    attroff(COLOR_PAIR(5));

    /* Controls */
    attron(COLOR_PAIR(4) | A_DIM);
    if (!game_over) {
        mvaddstr(screen_oy + 18, screen_ox + 4,
                 _("[H] Hit  [S] Stand  [D] Double  [Q] Quit",
                   "[H] Pedir  [S] Parar  [D] Dobrar  [Q] Sair"));
    }
    attroff(COLOR_PAIR(4) | A_DIM);
    refresh();
}

/* ───────── Main ───────── */
int main(void) {
    setlocale(LC_ALL, "");
    srand(time(NULL));
    read_config();
    initscr(); cbreak(); noecho(); curs_set(0);
    keypad(stdscr, TRUE); nodelay(stdscr, TRUE);
    start_color();

    init_theme_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_theme_pair(2, COLOR_YELLOW, COLOR_BLACK);
    init_theme_pair(3, COLOR_RED, COLOR_BLACK);
    init_theme_pair(4, COLOR_WHITE, COLOR_BLACK);
    init_theme_pair(5, COLOR_CYAN, COLOR_BLACK);
    init_theme_pair(6, COLOR_MAGENTA, COLOR_BLACK);
    apply_theme_bg();

    screen_ox = (COLS - 48) / 2;
    screen_oy = (LINES - 20) / 2;
    if (screen_ox < 1) screen_ox = 1;
    if (screen_oy < 1) screen_oy = 1;

    build_shoe();
    wins = losses = pushes = round_num = 0;

    int play = 1;
    init_round();

    while (play) {
        draw_game();

        int ch = getch();
        if (ch == 'q') { play = 0; break; }

        if (game_over) {
            if (ch == '\n' || ch == ' ') {
                init_round();
                continue;
            }
            continue;
        }

        if (ch == 'h' || ch == 'H') {
            deal_to(&player);
            if (hand_val(&player) > 21) {
                game_over = 1; result = 2; losses++;
            }
        } else if (ch == 's' || ch == 'S') {
            resolve();
        } else if (ch == 'd' || ch == 'D') {
            /* Double down: one card only, then stand */
            deal_to(&player);
            if (hand_val(&player) > 21) {
                game_over = 1; result = 2; losses++;
            } else {
                resolve();
            }
        }

        usleep(16000);
    }

    endwin();
    return 0;
}
