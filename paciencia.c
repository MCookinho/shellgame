/******************************************************************************
 *                          PACiENCIA (Klondike Solitaire)
 *                       Classic card game in the terminal
 ******************************************************************************/
#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include "config.h"

#define SUITS 4
#define RANKS 13
#define TOTAL 52

typedef struct { int suit, rank, up; } Card;

typedef struct { Card c[TOTAL]; int n; } Pile;

static Pile stock, waste, foundation[4], tableau[7];
static int sel_type = -1, sel_idx = -1, sel_card = -1;
static int cursor = 0;
static int won = 0, score = 0;

static const char *SUIT_CH = "♠♣♥♦";
static const char *RANK_CH = " A234567890JQK";

// Color scheme: suits 0-1 (♠♣) black, 2-3 (♥♦) red
// card pairs: 1=black, 2=red, 3=face_down, 4=highlight
static void card_str(Card c, char *buf) {
    if (!c.up) { strcpy(buf, "▓▓▓"); return; }
    char s = SUIT_CH[c.suit];
    if (c.rank == 10) { buf[0] = '1'; buf[1] = '0'; buf[2] = s; buf[3] = 0; }
    else { buf[0] = RANK_CH[c.rank]; buf[1] = s; buf[2] = ' '; buf[3] = 0; }
}

static void shuffle(Card *c, int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        Card t = c[i]; c[i] = c[j]; c[j] = t;
    }
}

static void init(void) {
    stock.n = waste.n = 0;
    for (int i = 0; i < 4; i++) foundation[i].n = 0;
    for (int i = 0; i < 7; i++) tableau[i].n = 0;

    Card deck[TOTAL];
    for (int s = 0; s < SUITS; s++)
        for (int r = 1; r <= RANKS; r++)
            deck[s * RANKS + (r - 1)] = (Card){ s, r, 0 };
    shuffle(deck, TOTAL);

    int idx = 0;
    for (int col = 0; col < 7; col++) {
        for (int r = 0; r <= col; r++) {
            Card c = deck[idx++];
            c.up = (r == col);
            tableau[col].c[tableau[col].n++] = c;
        }
    }
    while (idx < TOTAL) stock.c[stock.n++] = deck[idx++];
}

static int red(int suit) { return suit >= 2; }

static int can_tableau(Card c, Pile *p) {
    if (p->n == 0) return c.rank == 13;
    Card t = p->c[p->n - 1];
    return t.up && (red(c.suit) != red(t.suit)) && c.rank == t.rank - 1;
}

static int can_foundation(Card c, Pile *p) {
    if (p->n == 0) return c.rank == 1;
    Card t = p->c[p->n - 1];
    return c.suit == t.suit && c.rank == t.rank + 1;
}

static int try_move(int fk, int fi, int tk, int ti, int cn) {
    Pile *src = NULL, *dst = NULL;
    if (fk == 0) src = &waste;
    else if (fk == 1) src = &tableau[fi];

    if (tk == 0) dst = &foundation[ti];
    else if (tk == 1) dst = &tableau[ti];

    if (!src || !dst || src->n == 0) return 0;
    if (fk == 1 && fi == ti && tk == 1) return 0;

    Card moving = src->c[cn];

    if (tk == 0) {
        if (!can_foundation(moving, dst)) return 0;
        if (fk == 0) {
            dst->c[dst->n++] = src->c[--src->n];
            score += 10;
        } else {
            if (src->n - cn == 1) {
                dst->c[dst->n++] = src->c[--src->n];
                score += 10;
            } else return 0;
        }
    } else {
        if (!can_tableau(moving, dst)) return 0;
        int count = src->n - cn;
        for (int i = 0; i < count; i++)
            dst->c[dst->n++] = src->c[cn + i];
        src->n -= count;
        score += 5;
    }

    if (src->n > 0) src->c[src->n - 1].up = 1;
    return 1;
}

static void draw_card_at(char *s, int y, int x, int hl, int suit, int fd) {
    if (hl) attron(A_REVERSE);
    if (fd) { attron(COLOR_PAIR(3) | A_DIM); mvaddstr(y, x, s); attroff(COLOR_PAIR(3) | A_DIM); }
    else if (suit < 2) { attron(COLOR_PAIR(1) | A_BOLD); mvaddstr(y, x, s); attroff(COLOR_PAIR(1) | A_BOLD); }
    else { attron(COLOR_PAIR(2) | A_BOLD); mvaddstr(y, x, s); attroff(COLOR_PAIR(2) | A_BOLD); }
    if (hl) attroff(A_REVERSE);
}

static void try_auto_foundation(void) {
    for (int iter = 0; iter < 10; iter++) {
        for (int col = 0; col < 7; col++) {
            Pile *t = &tableau[col];
            if (t->n == 0) continue;
            Card c = t->c[t->n - 1];
            if (!c.up) continue;
            for (int f = 0; f < 4; f++)
                if (can_foundation(c, &foundation[f])) {
                    foundation[f].c[foundation[f].n++] = t->c[--t->n];
                    score += 10;
                    if (t->n > 0) t->c[t->n - 1].up = 1;
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
    nodelay(stdscr, FALSE);
    start_color();

    init_theme_pair(1, COLOR_WHITE, COLOR_BLACK);
    init_theme_pair(2, COLOR_RED, COLOR_BLACK);
    init_theme_pair(3, COLOR_BLUE, COLOR_BLACK);
    init_theme_pair(4, COLOR_CYAN, COLOR_BLACK);
    init_theme_pair(5, COLOR_GREEN, COLOR_BLACK);
    init_theme_pair(6, COLOR_YELLOW, COLOR_BLACK);
    apply_theme_bg();

    int play_again = 1;
    while (play_again) {
        init();
        won = 0; score = 0;
        sel_type = sel_idx = sel_card = -1;
        cursor = 0;

        int ch;
        while (!won) {
            // Check win
            int all_done = 1;
            for (int i = 0; i < 4; i++)
                if (foundation[i].n != 13) { all_done = 0; break; }
            if (all_done) { won = 1; break; }

            erase();

            // Title
            attron(COLOR_PAIR(4) | A_BOLD);
            mvaddstr(0, 2, _("SOLITAIRE", "PACIENCIA"));
            attroff(COLOR_PAIR(4) | A_BOLD);

            // Stock
            int cx = 2;
            mvaddstr(0, cx + 1, "S");
            if (stock.n > 0) draw_card_at("▓▓▓", 1, cx, cursor == 0, 0, 1);
            else mvaddstr(1, cx, "   ");
            char buf[8]; snprintf(buf, sizeof buf, " %d", stock.n);
            mvaddstr(2, cx, buf);

            cx += 6;
            mvaddstr(0, cx + 1, "W");
            if (waste.n > 0) {
                Card top = waste.c[waste.n - 1];
                char s[16]; card_str(top, s);
                draw_card_at(s, 1, cx, cursor == 1, top.suit, 0);
            } else mvaddstr(1, cx, "   ");

            cx += 8;
            char *fn[] = { "1♠", "2♣", "3♥", "4♦" };
            for (int i = 0; i < 4; i++) {
                attron(COLOR_PAIR(5)); mvaddstr(0, cx + i * 6, fn[i]); attroff(COLOR_PAIR(5));
                if (foundation[i].n > 0) {
                    Card t = foundation[i].c[foundation[i].n - 1];
                    char s[16]; card_str(t, s);
                    draw_card_at(s, 1, cx + i * 6, cursor == 2 + i, t.suit, 0);
                } else mvaddstr(1, cx + i * 6, "   ");
            }

            // Divider
            mvhline(3, 0, ' ', COLS);

            // Tableau
            int max_vis = 12;
            for (int col = 0; col < 7; col++) {
                int col_x = 2 + col * 6;
                attron(COLOR_PAIR(6)); mvprintw(4, col_x, "%d", col + 1); attroff(COLOR_PAIR(6));

                Pile *p = &tableau[col];
                int vis = p->n > max_vis ? max_vis : p->n;
                int start = p->n - vis;
                for (int r = 0; r < vis; r++) {
                    char s[16]; card_str(p->c[start + r], s);
                    int hl = (cursor == 6 + col) && (sel_type == -1 || (sel_type == 1 && sel_idx == col));
                    draw_card_at(s, 5 + r, col_x, hl, p->c[start + r].suit, !p->c[start + r].up);
                }
            }

            // Info
            attron(COLOR_PAIR(4));
            if (sel_type >= 0) mvaddstr(5 + 14, 2, _("Selected. Enter: place | Esc: cancel", "Selecionado. Enter: colocar | Esc: cancelar"));
            char sc[64];
            snprintf(sc, sizeof sc, "Score: %d", score);
            mvaddstr(5 + 15, 2, sc);
            mvaddstr(5 + 16, 2, _("Arrows: move | Enter: select/place | Space: draw | Q: quit", "Setas: mover | Enter: sel/colocar | Espaco: virar | Q: sair"));
            attroff(COLOR_PAIR(4));

            refresh();

            ch = getch();
            if (ch == 'q') { play_again = 0; break; }

            // Navigation
            if (ch == KEY_LEFT && cursor > 0) cursor--;
            if (ch == KEY_RIGHT && cursor < 12) cursor++;
            if (ch == KEY_UP && cursor >= 6) cursor = cursor % 2;
            if (ch == KEY_DOWN && cursor < 6) cursor = 6 + (cursor % 7);
            if (cursor > 12) cursor = 12;

            // Space - draw stock
            if (ch == ' ') {
                if (stock.n > 0) {
                    Card c = stock.c[--stock.n];
                    c.up = 1;
                    waste.c[waste.n++] = c;
                } else if (waste.n > 0) {
                    while (waste.n > 0) {
                        Card c = waste.c[--waste.n];
                        c.up = 0;
                        stock.c[stock.n++] = c;
                    }
                }
            }

            // Enter - select/place
            if (ch == '\n') {
                if (sel_type == -1) {
                    if (cursor == 1 && waste.n > 0) {
                        sel_type = 0; sel_idx = 0; sel_card = waste.n - 1;
                    } else if (cursor >= 6) {
                        int col = cursor - 6;
                        Pile *p = &tableau[col];
                        if (p->n > 0) {
                            sel_type = 1; sel_idx = col;
                            for (int i = 0; i < p->n; i++)
                                if (p->c[i].up) { sel_card = i; break; }
                        }
                    }
                } else {
                    int ok = 0;
                    if (cursor >= 2 && cursor <= 5)
                        ok = try_move(sel_type, sel_idx, 0, cursor - 2, sel_card);
                    else if (cursor >= 6)
                        ok = try_move(sel_type, sel_idx, 1, cursor - 6, sel_card);
                    if (ok) try_auto_foundation();
                    sel_type = sel_idx = sel_card = -1;
                }
            }

            if (ch == 27) { sel_type = sel_idx = sel_card = -1; }
        }

        if (play_again) {
            erase();
            if (won) {
                attron(COLOR_PAIR(5) | A_BOLD);
                mvaddstr(5, 10, _("CONGRATULATIONS! YOU WIN!", "PARABENS! VOCE VENCEU O PACIENCIA!"));
                attroff(COLOR_PAIR(5) | A_BOLD);
                score += 1000;
            }
            attron(COLOR_PAIR(4));
            char buf[64];
            snprintf(buf, sizeof buf, _("Final score: %d", "Pontuacao final: %d"), score);
            mvaddstr(7, 10, buf);
            mvaddstr(9, 6, _("Enter: play again | Q: quit", "Enter: jogar novamente | Q: sair"));
            attroff(COLOR_PAIR(4));
            refresh();
            while (1) {
                ch = getch();
                if (ch == 'q') { play_again = 0; break; }
                if (ch == '\n') { play_again = 1; break; }
            }
        }
    }

    endwin();
    return 0;
}
