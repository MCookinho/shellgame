/******************************************************************************
 *                              CHECKERS
 *       Classic draughts for two players in the terminal.
 ******************************************************************************/
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <locale.h>
#include "config.h"

enum { EMPTY, MAN, KING };
enum { WHITE, BLACK };
static const char *piece_ch[2][3] = {
    {"\xe2\x9b\x81", "\xe2\x9b\x80", ""},  /* white man, white king */
    {"\xe2\x9b\x83", "\xe2\x9b\x82", ""},  /* black man, black king */
};

typedef struct { int type, color; } Square;
static Square board[8][8];
static int turn, game_over, result;
static int cur_r, cur_c, sel_r, sel_c, selected;
static int must_cap, must_r, must_c;
static int forced_capture; /* any_capture(turn) flag for UI */

static int wins[2], rounds;

/* ───────── Init ───────── */
static void init_board(void) {
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++)
            board[r][c].type = EMPTY;

    for (int r = 0; r < 3; r++)
        for (int c = 0; c < 8; c++)
            if ((r + c) % 2)
                board[r][c].type = MAN, board[r][c].color = BLACK;

    for (int r = 5; r < 8; r++)
        for (int c = 0; c < 8; c++)
            if ((r + c) % 2)
                board[r][c].type = MAN, board[r][c].color = WHITE;

    turn = WHITE;
    game_over = result = 0;
    must_cap = forced_capture = 0;
    cur_r = 0; cur_c = 0;
    selected = 0;
}

/* ───────── Helpers ───────── */
static int inb(int r, int c) { return r >= 0 && r < 8 && c >= 0 && c < 8; }
static int dark(int r, int c) { return (r + c) % 2; }
static int enemy(int c) { return c == WHITE ? BLACK : WHITE; }
static int forward(int color) { return color == WHITE ? -1 : 1; }

/* ───────── Can piece at (r,c) capture? ───────── */
static int can_capture_from(int r, int c) {
    if (!inb(r,c) || board[r][c].type == EMPTY) return 0;
    int col = board[r][c].color;
    int dirs[4][2] = {{-1,-1},{-1,1},{1,-1},{1,1}};
    int n = (board[r][c].type == KING) ? 4 : 2;
    int sd = (board[r][c].type == KING) ? 0 : ((col == WHITE) ? 0 : 2);
    for (int i = sd; i < sd + n; i++) {
        int nr = r + dirs[i][0], nc = c + dirs[i][1];
        int lr = r + 2*dirs[i][0], lc = c + 2*dirs[i][1];
        if (!inb(lr,lc)) continue;
        if (board[nr][nc].type == EMPTY) continue;
        if (board[nr][nc].color != enemy(col)) continue;
        if (board[lr][lc].type != EMPTY) continue;
        return 1;
    }
    return 0;
}

/* ───────── Any capture available for color? ───────── */
static int any_capture(int color) {
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++)
            if (board[r][c].type != EMPTY && board[r][c].color == color)
                if (can_capture_from(r, c)) return 1;
    return 0;
}

/* ───────── Can piece move to target? ───────── */
static int can_move(int fr, int fc, int tr, int tc) {
    if (!inb(fr,fc) || !inb(tr,tc)) return 0;
    Square *f = &board[fr][fc], *t = &board[tr][tc];
    if (f->type == EMPTY) return 0;
    if (t->type != EMPTY) return 0;
    if (!dark(tr,tc)) return 0;

    int dr = tr - fr, dc = abs(tc - fc);
    int is_king = (f->type == KING);
    int fd = forward(f->color);

    /* Must capture if possible */
    int must = (must_cap || forced_capture);

    /* Simple move (1 square diagonal) */
    if (dr == fd && dc == 1 && !is_king) {
        if (must) return 0;
        return 1;
    }
    if (is_king && dc == 1 && abs(dr) == 1) {
        if (must) return 0;
        return 1;
    }

    /* Capture (2 squares diagonal) */
    if (dc == 2 && abs(dr) == 2) {
        int mr = fr + dr/2, mc = fc + (tc-fc)/2;
        if (!inb(mr,mc)) return 0;
        if (board[mr][mc].type == EMPTY) return 0;
        if (board[mr][mc].color != enemy(f->color)) return 0;
        if (!is_king && dr * fd <= 0) return 0;
        if (must_cap && (fr != must_r || fc != must_c)) return 0;
        return 1;
    }
    return 0;
}

/* ───────── Execute move ───────── */
static void exec_move(int fr, int fc, int tr, int tc) {
    int dr = tr - fr;
    int is_cap = (abs(dr) == 2);

    board[tr][tc] = board[fr][fc];
    board[fr][fc].type = EMPTY;

    if (is_cap) {
        int mr = fr + dr/2, mc = fc + (tc-fc)/2;
        board[mr][mc].type = EMPTY;

        /* Check king row promotion (stop jump chain) */
        int prom = (board[tr][tc].color == WHITE && tr == 0)
                || (board[tr][tc].color == BLACK && tr == 7);
        if (prom)
            board[tr][tc].type = KING;

        /* Check for multi-jump */
        if (!prom && can_capture_from(tr, tc)) {
            must_cap = 1;
            must_r = tr; must_c = tc;
            return; /* player continues */
        }
    }

    /* King promotion on simple move */
    if (!is_cap) {
        if (board[tr][tc].color == WHITE && tr == 0)
            board[tr][tc].type = KING;
        if (board[tr][tc].color == BLACK && tr == 7)
            board[tr][tc].type = KING;
    }

    must_cap = 0;
    turn = enemy(turn);
}

/* ───────── Check end conditions ───────── */
static void check_end(void) {
    int w = 0, b = 0;
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++) {
            if (board[r][c].type == EMPTY) continue;
            if (board[r][c].color == WHITE) w++;
            else b++;
        }
    if (w == 0) { game_over = 1; result = 2; wins[BLACK]++; }
    if (b == 0) { game_over = 1; result = 1; wins[WHITE]++; }

    /* Also check if current player has no moves */
    if (!game_over) {
        int any = 0;
        for (int r = 0; r < 8; r++)
            for (int c = 0; c < 8; c++) {
                if (board[r][c].type == EMPTY || board[r][c].color != turn) continue;
                for (int tr = 0; tr < 8 && !any; tr++)
                    for (int tc = 0; tc < 8 && !any; tc++)
                        if (can_move(r, c, tr, tc)) any = 1;
            }
        if (!any) {
            game_over = 1;
            result = (turn == WHITE) ? 2 : 1;
            wins[turn == WHITE ? BLACK : WHITE]++;
        }
    }
}

/* ───────── Draw ───────── */
static void draw(int ox, int oy) {
    erase();

    attron(COLOR_PAIR(2) | A_BOLD);
    mvaddstr(oy, ox + 16, _("CHECKERS", "DAMAS"));
    attroff(COLOR_PAIR(2) | A_BOLD);

    char buf[64];
    snprintf(buf, 64, _("Round: %d  W: %d  B: %d",
                         "Rodada: %d  B: %d  P: %d"),
             rounds, wins[WHITE], wins[BLACK]);
    attron(COLOR_PAIR(5));
    mvaddstr(oy, ox + 28, buf);
    attroff(COLOR_PAIR(5));

    /* Column labels */
    attron(COLOR_PAIR(4) | A_DIM);
    for (int c = 0; c < 8; c++)
        mvaddch(oy + 1, ox + 2 + c * 2, 'a' + c);
    for (int c = 0; c < 8; c++)
        mvaddch(oy + 10, ox + 2 + c * 2, 'a' + c);
    for (int r = 0; r < 8; r++)
        mvaddch(oy + r + 2, ox, '1' + r);
    for (int r = 0; r < 8; r++)
        mvaddch(oy + r + 2, ox + 18, '1' + r);
    attroff(COLOR_PAIR(4) | A_DIM);

    /* Board */
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            int y = oy + r + 2;
            int x = ox + 2 + c * 2;
            int is_dark = dark(r, c);
            int is_sel = selected && r == sel_r && c == sel_c;
            int is_cur = !selected && r == cur_r && c == cur_c;
            int valid = selected && can_move(sel_r, sel_c, r, c);
            int hi_jump = must_cap && r == must_r && c == must_c;
            int can_cap_here = forced_capture && !must_cap
                               && board[r][c].type != EMPTY
                               && board[r][c].color == turn
                               && can_capture_from(r, c);

            if (is_sel)
                attron(COLOR_PAIR(2) | A_REVERSE);
            else if (hi_jump)
                attron(COLOR_PAIR(3) | A_REVERSE);
            else if (is_cur)
                attron(A_REVERSE);
            else if (can_cap_here)
                attron(COLOR_PAIR(3) | A_REVERSE);
            else if (valid)
                attron(COLOR_PAIR(1) | A_REVERSE);

            if (board[r][c].type != EMPTY) {
                int col = (board[r][c].color == WHITE) ? 4 : 3;
                attron(COLOR_PAIR(col) | A_BOLD);

                int idx = (board[r][c].type == KING) ? 1 : 0;
                mvaddstr(y, x, piece_ch[board[r][c].color][idx]);

                attroff(COLOR_PAIR(col) | A_BOLD);
            } else if (is_dark) {
                attron(COLOR_PAIR(4) | A_DIM);
                mvaddstr(y, x, "\xc2\xb7");
                attroff(COLOR_PAIR(4) | A_DIM);
            } else {
                mvaddstr(y, x, " ");
            }

            attroff(COLOR_PAIR(2) | A_REVERSE | A_REVERSE);
            attroff(COLOR_PAIR(3) | A_REVERSE);
            attroff(COLOR_PAIR(1) | A_REVERSE);
        }
    }

    /* Info panel */
    int ix = ox + 22;
    attron(COLOR_PAIR(turn == WHITE ? 4 : 3) | A_BOLD);
    snprintf(buf, 64, _("Turn: %s", "Vez: %s"),
             turn == WHITE ? _("White", "Branco") : _("Black", "Preto"));
    mvaddstr(oy + 2, ix, buf);
    attroff(COLOR_PAIR(turn == WHITE ? 4 : 3) | A_BOLD);

    if (must_cap || forced_capture) {
        attron(COLOR_PAIR(3) | A_BOLD);
        mvaddstr(oy + 3, ix, _("CAPTURE REQUIRED!", "CAPTURA OBRIGATORIA!"));
        attroff(COLOR_PAIR(3) | A_BOLD);
    }

    if (selected) {
        attron(COLOR_PAIR(5));
        char pbuf[32];
        snprintf(pbuf, 32, _("Selected: %c%c", "Selecionado: %c%c"),
                 'a'+sel_c, '1'+sel_r);
        mvaddstr(oy + 5, ix, pbuf);
        /* count moves */
        int cnt = 0;
        for (int r = 0; r < 8; r++)
            for (int c = 0; c < 8; c++)
                if (can_move(sel_r, sel_c, r, c)) cnt++;
        snprintf(pbuf, 32, _("Moves: %d", "Mov: %d"), cnt);
        mvaddstr(oy + 6, ix, pbuf);
        attroff(COLOR_PAIR(5));
    }

    if (game_over) {
        attron(COLOR_PAIR(2) | A_BOLD);
        if (result == 1)
            mvaddstr(oy + 8, ix, _("White wins!", "Branco vence!"));
        else
            mvaddstr(oy + 8, ix, _("Black wins!", "Preto vence!"));
        attroff(COLOR_PAIR(2) | A_BOLD);

        attron(COLOR_PAIR(6) | A_DIM);
        mvaddstr(oy + 10, ix,
                 _("Enter: next game  Q: quit",
                   "Enter: novo jogo  Q: sair"));
        attroff(COLOR_PAIR(6) | A_DIM);
    }

    attron(COLOR_PAIR(4) | A_DIM);
    mvaddstr(oy + 14, ix,
             _("Arrows: move  Enter: select/move",
               "Setas: mover  Enter: selecionar/mover"));
    mvaddstr(oy + 15, ix, _("Q: quit", "Q: sair"));
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

    wins[WHITE] = wins[BLACK] = 0;
    rounds = 0;

    int play = 1;
    while (play) {
        init_board();
        rounds++;

        while (1) {
            if (!must_cap) forced_capture = any_capture(turn);
            check_end();
            draw((COLS - 44) / 2 < 1 ? 1 : (COLS - 44) / 2,
                 (LINES - 16) / 2 < 1 ? 1 : (LINES - 16) / 2);

            int ch = getch();
            if (ch == 'q') { play = 0; break; }
            if (game_over) {
                if (ch == '\n' || ch == ' ') break;
                continue;
            }

            /* Cursor */
            int nr = cur_r, nc = cur_c;
            if (ch == KEY_UP || ch == 'w') nr--;
            else if (ch == KEY_DOWN || ch == 's') nr++;
            else if (ch == KEY_LEFT || ch == 'a') nc--;
            else if (ch == KEY_RIGHT || ch == 'd') nc++;
            if (inb(nr, nc)) { cur_r = nr; cur_c = nc; }

            /* Select / move */
            if (ch == '\n' || ch == ' ') {
                if (!selected) {
                    if (board[cur_r][cur_c].type != EMPTY
                        && board[cur_r][cur_c].color == turn) {
                        if (must_cap && (cur_r != must_r || cur_c != must_c))
                            continue;
                        if (!must_cap && forced_capture
                            && !can_capture_from(cur_r, cur_c))
                            continue;
                        sel_r = cur_r; sel_c = cur_c;
                        selected = 1;
                    }
                } else {
                    if (cur_r == sel_r && cur_c == sel_c) {
                        selected = 0;
                    } else if (can_move(sel_r, sel_c, cur_r, cur_c)) {
                        exec_move(sel_r, sel_c, cur_r, cur_c);
                        selected = 0;
                    } else {
                        if (board[cur_r][cur_c].type != EMPTY
                            && board[cur_r][cur_c].color == turn) {
                            int ok = 1;
                            if (must_cap && (cur_r != must_r || cur_c != must_c))
                                ok = 0;
                            if (!must_cap && forced_capture
                                && !can_capture_from(cur_r, cur_c))
                                ok = 0;
                            if (ok) { sel_r = cur_r; sel_c = cur_c; }
                        }
                    }
                }
            }

            usleep(16000);
        }
        if (!play) break;
    }

    endwin();
    return 0;
}
