/******************************************************************************
 *                              CHESS
 *     Classic chess for two players in the terminal.
 ******************************************************************************/
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <locale.h>
#include "config.h"

/* ───────── Pieces ───────── */
enum { EMPTY, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING };
enum { WHITE, BLACK };

static const char *piece_utf[2][7] = {
    {"", "\xe2\x99\x99", "\xe2\x99\x98", "\xe2\x99\x97",
         "\xe2\x99\x96", "\xe2\x99\x95", "\xe2\x99\x94"},
    {"", "\xe2\x99\x9f", "\xe2\x99\x9e", "\xe2\x99\x9d",
         "\xe2\x99\x9c", "\xe2\x99\x9b", "\xe2\x99\x9a"},
};
static const char *piece_asc[2][7] = {
    {"", "P", "N", "B", "R", "Q", "K"},
    {"", "p", "n", "b", "r", "q", "k"},
};

static int use_unicode = 1;

/* ───────── Board ───────── */
typedef struct { int type, color; } Square;
static Square board[8][8];
static int turn; /* WHITE or BLACK */
static int game_over;
static int result; /* 0=playing, 1=white_wins, 2=black_wins, 3=draw */

/* castling rights */
static int castle_wk, castle_wq, castle_bk, castle_bq;
/* en passant target: -1 if none, else file */
static int ep_target;
/* halfmove clock for 50-move rule (not implemented fully) */
static int halfmove;
static int fullmove;

/* cursor */
static int cur_r, cur_c;
static int sel_r, sel_c, selected;

/* move list for undo (just for display) */
typedef struct { char from[3], to[3]; char piece; int captured; } Move;
static Move history[256];
static int hcount;

/* ───────── Init board ───────── */
static void init_board(void) {
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++)
            board[r][c].type = EMPTY, board[r][c].color = WHITE;

    int back[8] = {ROOK, KNIGHT, BISHOP, QUEEN, KING, BISHOP, KNIGHT, ROOK};
    for (int c = 0; c < 8; c++) {
        board[0][c].type = back[c]; board[0][c].color = BLACK;
        board[1][c].type = PAWN;    board[1][c].color = BLACK;
        board[6][c].type = PAWN;    board[6][c].color = WHITE;
        board[7][c].type = back[c]; board[7][c].color = WHITE;
    }

    turn = WHITE;
    game_over = 0; result = 0;
    castle_wk = castle_wq = castle_bk = castle_bq = 1;
    ep_target = -1;
    halfmove = 0; fullmove = 1;
    cur_r = 7; cur_c = 4; /* king start */
    selected = 0;
    hcount = 0;
}

/* ───────── In bounds ───────── */
static int inb(int r, int c) { return r >= 0 && r < 8 && c >= 0 && c < 8; }

/* ───────── Find king ───────── */
static void find_king(int color, int *kr, int *kc) {
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++)
            if (board[r][c].type == KING && board[r][c].color == color)
                { *kr = r; *kc = c; return; }
}

/* ───────── Is square attacked? ───────── */
static int is_attacked(int r, int c, int by_color) {
    /* pawns */
    int d = (by_color == WHITE) ? -1 : 1;
    if (inb(r + d, c - 1) && board[r + d][c - 1].type == PAWN
        && board[r + d][c - 1].color == by_color) return 1;
    if (inb(r + d, c + 1) && board[r + d][c + 1].type == PAWN
        && board[r + d][c + 1].color == by_color) return 1;
    /* knights */
    int kd[] = {-2,-1, -2,1, -1,-2, -1,2, 1,-2, 1,2, 2,-1, 2,1};
    for (int i = 0; i < 16; i += 2)
        if (inb(r + kd[i], c + kd[i+1])
            && board[r + kd[i]][c + kd[i+1]].type == KNIGHT
            && board[r + kd[i]][c + kd[i+1]].color == by_color) return 1;
    /* king */
    for (int dr = -1; dr <= 1; dr++)
        for (int dc = -1; dc <= 1; dc++) {
            if (dr == 0 && dc == 0) continue;
            if (inb(r + dr, c + dc)
                && board[r + dr][c + dc].type == KING
                && board[r + dr][c + dc].color == by_color) return 1;
        }
    /* rook/queen (straight lines) */
    int dirs[] = {-1,0, 1,0, 0,-1, 0,1};
    for (int i = 0; i < 8; i += 2) {
        for (int k = 1; ; k++) {
            int nr = r + dirs[i] * k, nc = c + dirs[i+1] * k;
            if (!inb(nr, nc)) break;
            if (board[nr][nc].type == EMPTY) continue;
            if ((board[nr][nc].type == ROOK || board[nr][nc].type == QUEEN)
                && board[nr][nc].color == by_color) return 1;
            break;
        }
    }
    /* bishop/queen (diagonals) */
    int ddirs[] = {-1,-1, -1,1, 1,-1, 1,1};
    for (int i = 0; i < 8; i += 2) {
        for (int k = 1; ; k++) {
            int nr = r + ddirs[i] * k, nc = c + ddirs[i+1] * k;
            if (!inb(nr, nc)) break;
            if (board[nr][nc].type == EMPTY) continue;
            if ((board[nr][nc].type == BISHOP || board[nr][nc].type == QUEEN)
                && board[nr][nc].color == by_color) return 1;
            break;
        }
    }
    return 0;
}

/* ───────── In check? ───────── */
static int in_check(int color) {
    int kr = 0, kc = 0;
    find_king(color, &kr, &kc);
    return is_attacked(kr, kc, !color);
}

/* ───────── Simulate move and test ───────── */
static int would_be_check(int fr, int fc, int tr, int tc, int color) {
    Square save_from = board[fr][fc];
    Square save_to = board[tr][tc];
    board[tr][tc] = board[fr][fc];
    board[fr][fc].type = EMPTY;
    int check = in_check(color);
    board[fr][fc] = save_from;
    board[tr][tc] = save_to;
    return check;
}

/* ───────── Valid moves for a square ───────── */
static int is_valid_move(int fr, int fc, int tr, int tc) {
    if (!inb(fr, fc) || !inb(tr, tc)) return 0;
    Square *f = &board[fr][fc];
    Square *t = &board[tr][tc];
    if (f->type == EMPTY) return 0;
    if (t->type != EMPTY && t->color == f->color) return 0;

    int ok = 0;
    int d = (f->color == WHITE) ? -1 : 1;
    int start_r = (f->color == WHITE) ? 6 : 1;

    switch (f->type) {
        case PAWN: {
            /* forward */
            if (tc == fc && t->type == EMPTY) {
                if (tr == fr + d) ok = 1;
                else if (tr == fr + 2 * d && fr == start_r
                         && board[fr + d][fc].type == EMPTY) ok = 1;
            }
            /* capture */
            if (abs(tc - fc) == 1 && tr == fr + d && t->type != EMPTY
                && t->color != f->color) ok = 1;
            /* en passant */
            if (abs(tc - fc) == 1 && tr == fr + d && t->type == EMPTY
                && ep_target == tc) ok = 1;
            break;
        }
        case KNIGHT: {
            int dr = abs(tr - fr), dc = abs(tc - fc);
            if ((dr == 2 && dc == 1) || (dr == 1 && dc == 2)) ok = 1;
            break;
        }
        case BISHOP: {
            if (abs(tr - fr) == abs(tc - fc)) {
                ok = 1;
                int dr = (tr > fr) ? 1 : -1;
                int dc = (tc > fc) ? 1 : -1;
                for (int r = fr + dr, c = fc + dc;
                     r != tr && c != tc; r += dr, c += dc)
                    if (board[r][c].type != EMPTY) { ok = 0; break; }
            }
            break;
        }
        case ROOK: {
            if (tr == fr || tc == fc) {
                ok = 1;
                int dr = (tr > fr) ? 1 : (tr < fr) ? -1 : 0;
                int dc = (tc > fc) ? 1 : (tc < fc) ? -1 : 0;
                for (int r = fr + dr, c = fc + dc;
                     r != tr || c != tc; r += dr, c += dc)
                    if (board[r][c].type != EMPTY) { ok = 0; break; }
            }
            break;
        }
        case QUEEN: {
            if (tr == fr || tc == fc || abs(tr - fr) == abs(tc - fc)) {
                ok = 1;
                int dr = (tr > fr) ? 1 : (tr < fr) ? -1 : 0;
                int dc = (tc > fc) ? 1 : (tc < fc) ? -1 : 0;
                for (int r = fr + dr, c = fc + dc;
                     r != tr || c != tc; r += dr, c += dc)
                    if (board[r][c].type != EMPTY) { ok = 0; break; }
            }
            break;
        }
        case KING: {
            if (abs(tr - fr) <= 1 && abs(tc - fc) <= 1) ok = 1;
            /* castling */
            if (tr == fr && abs(tc - fc) == 2) {
                if (f->color == WHITE) {
                    if (tc == 6 && castle_wk
                        && board[7][7].type == ROOK && board[7][7].color == WHITE
                        && board[7][5].type == EMPTY && board[7][6].type == EMPTY
                        && !is_attacked(7,4,BLACK) && !is_attacked(7,5,BLACK)
                        && !is_attacked(7,6,BLACK)) ok = 1;
                    if (tc == 2 && castle_wq
                        && board[7][0].type == ROOK && board[7][0].color == WHITE
                        && board[7][1].type == EMPTY && board[7][2].type == EMPTY
                        && board[7][3].type == EMPTY
                        && !is_attacked(7,4,BLACK) && !is_attacked(7,3,BLACK)
                        && !is_attacked(7,2,BLACK)) ok = 1;
                } else {
                    if (tc == 6 && castle_bk
                        && board[0][7].type == ROOK && board[0][7].color == BLACK
                        && board[0][5].type == EMPTY && board[0][6].type == EMPTY
                        && !is_attacked(0,4,WHITE) && !is_attacked(0,5,WHITE)
                        && !is_attacked(0,6,WHITE)) ok = 1;
                    if (tc == 2 && castle_bq
                        && board[0][0].type == ROOK && board[0][0].color == BLACK
                        && board[0][1].type == EMPTY && board[0][2].type == EMPTY
                        && board[0][3].type == EMPTY
                        && !is_attacked(0,4,WHITE) && !is_attacked(0,3,WHITE)
                        && !is_attacked(0,2,WHITE)) ok = 1;
                }
            }
            break;
        }
    }
    if (!ok) return 0;
    return !would_be_check(fr, fc, tr, tc, f->color);
}

/* ───────── Make move ───────── */
static void make_move(int fr, int fc, int tr, int tc) {
    Square *f = &board[fr][fc];
    Square *t = &board[tr][tc];
    int type = f->type;
    int color = f->color;

    /* save history */
    Move *m = &history[hcount++];
    m->piece = (color == WHITE ? 'P' : 'p') + type - 1;
    m->captured = t->type;
    snprintf(m->from, 3, "%c%d", 'a' + fc, 8 - fr);
    snprintf(m->to, 3, "%c%d", 'a' + tc, 8 - tr);

    /* handle en passant capture */
    if (type == PAWN && tc != fc && t->type == EMPTY)
        board[fr][tc].type = EMPTY;

    /* move piece */
    *t = *f;
    f->type = EMPTY;

    /* castling - move rook */
    if (type == KING && abs(tc - fc) == 2) {
        if (tc == 6) { board[tr][5] = board[tr][7]; board[tr][7].type = EMPTY; }
        if (tc == 2) { board[tr][3] = board[tr][0]; board[tr][0].type = EMPTY; }
    }

    /* pawn promotion */
    if (type == PAWN && (tr == 0 || tr == 7))
        t->type = QUEEN;

    /* update castling rights */
    if (type == KING) {
        if (color == WHITE) castle_wk = castle_wq = 0;
        else castle_bk = castle_bq = 0;
    }
    if (fr == 7 && fc == 0) castle_wq = 0;
    if (fr == 7 && fc == 7) castle_wk = 0;
    if (fr == 0 && fc == 0) castle_bq = 0;
    if (fr == 0 && fc == 7) castle_bk = 0;

    /* en passant target */
    ep_target = -1;
    if (type == PAWN && abs(tr - fr) == 2)
        ep_target = fc;

    turn = !turn;
    fullmove += (turn == WHITE) ? 1 : 0;
}

/* ───────── Check for end conditions ───────── */
static int has_legal_moves(int color) {
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++)
            if (board[r][c].type != EMPTY && board[r][c].color == color)
                for (int tr = 0; tr < 8; tr++)
                    for (int tc = 0; tc < 8; tc++)
                        if (is_valid_move(r, c, tr, tc)) return 1;
    return 0;
}

static void check_game_end(void) {
    int check = in_check(turn);
    int legal = has_legal_moves(turn);
    if (!legal) {
        game_over = 1;
        if (check) result = (turn == WHITE) ? 2 : 1;
        else result = 3;
    }
}

/* ───────── Draw board ───────── */
static void draw_board(int ox, int oy) {
    /* file labels */
    attron(COLOR_PAIR(4) | A_DIM);
    for (int c = 0; c < 8; c++)
        mvaddch(oy, ox + c * 2 + 2, 'a' + c);
    for (int c = 0; c < 8; c++)
        mvaddch(oy + 9, ox + c * 2 + 2, 'a' + c);
    /* rank labels */
    for (int r = 0; r < 8; r++)
        mvaddch(oy + r + 1, ox, '8' - r);
    for (int r = 0; r < 8; r++)
        mvaddch(oy + r + 1, ox + 18, '8' - r);
    attroff(COLOR_PAIR(4) | A_DIM);

    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            int y = oy + r + 1;
            int x = ox + c * 2 + 2;
            int dark = (r + c) % 2;
            int is_sel = (r == sel_r && c == sel_c && selected);
            int is_cur = (r == cur_r && c == cur_c);
            int valid = selected && is_valid_move(sel_r, sel_c, r, c);

            /* background */
            if (is_sel)
                attron(COLOR_PAIR(2) | A_REVERSE);
            else if (is_cur && !selected)
                attron(A_REVERSE);
            else if (valid)
                attron(COLOR_PAIR(1) | A_REVERSE);
            else
                attron(COLOR_PAIR(dark ? 4 : 0) | (dark ? A_DIM : A_NORMAL));

            /* piece or empty */
            Square *s = &board[r][c];
            if (s->type == EMPTY) {
                if (valid) mvaddstr(y, x, "·");
                else mvaddstr(y, x, dark ? "·" : " ");
            } else if (use_unicode) {
                mvaddstr(y, x, piece_utf[s->color][s->type]);
            } else {
                mvaddstr(y, x, piece_asc[s->color][s->type]);
            }

            attroff(COLOR_PAIR(dark ? 4 : 0) | (dark ? A_DIM : A_NORMAL)
                    | A_REVERSE);
            attroff(COLOR_PAIR(2) | A_REVERSE);
            attroff(COLOR_PAIR(1) | A_REVERSE);
        }
    }
}

/* ───────── Info panel ───────── */
static void draw_info(int ox, int oy) {
    int ix = ox + 22;
    attron(COLOR_PAIR(2) | A_BOLD);
    mvaddstr(oy + 1, ix, _("CHESS", "XADREZ"));
    attroff(COLOR_PAIR(2) | A_BOLD);

    char buf[64];
    snprintf(buf, 64, _("Turn: %s", "Vez: %s"),
             turn == WHITE ? _("White", "Branco") : _("Black", "Preto"));
    attron(COLOR_PAIR(turn == WHITE ? 4 : 0));
    mvaddstr(oy + 3, ix, buf);
    attroff(COLOR_PAIR(turn == WHITE ? 4 : 0));

    if (in_check(turn) && !game_over) {
        attron(COLOR_PAIR(3) | A_BOLD);
        mvaddstr(oy + 4, ix, _("CHECK!", "XEQUE!"));
        attroff(COLOR_PAIR(3) | A_BOLD);
    }

    if (game_over) {
        attron(COLOR_PAIR(2) | A_BOLD);
        if (result == 1)
            mvaddstr(oy + 6, ix, _("White wins!", "Branco vence!"));
        else if (result == 2)
            mvaddstr(oy + 6, ix, _("Black wins!", "Preto vence!"));
        else if (result == 3)
            mvaddstr(oy + 6, ix, _("Stalemate!", "Empate!"));
        attroff(COLOR_PAIR(2) | A_BOLD);

        attron(COLOR_PAIR(6) | A_DIM);
        mvaddstr(oy + 8, ix,
                 _("Enter: play again  Q: quit",
                   "Enter: jogar novamente  Q: sair"));
        attroff(COLOR_PAIR(6) | A_DIM);
    }

    /* selected piece info */
    if (selected) {
        Square *s = &board[sel_r][sel_c];
        if (s->type != EMPTY) {
            attron(COLOR_PAIR(5));
            char pstr[16];
            if (use_unicode)
                snprintf(pstr, 16, "%s (%c%d)", piece_utf[s->color][s->type],
                         'a'+sel_c, 8-sel_r);
            else
                snprintf(pstr, 16, "%s (%c%d)", piece_asc[s->color][s->type],
                         'a'+sel_c, 8-sel_r);
            mvaddstr(oy + 10, ix, pstr);
            /* count valid moves */
            int cnt = 0;
            for (int r = 0; r < 8; r++)
                for (int c = 0; c < 8; c++)
                    if (is_valid_move(sel_r, sel_c, r, c)) cnt++;
            char cbuf[16];
            snprintf(cbuf, 16, _("Moves: %d", "Mov: %d"), cnt);
            mvaddstr(oy + 11, ix, cbuf);
            attroff(COLOR_PAIR(5));
        }
    }

    /* move history (last 5) */
    attron(COLOR_PAIR(4) | A_DIM);
    mvaddstr(oy + 13, ix, _("Moves:", "Jogadas:"));
    attroff(COLOR_PAIR(4) | A_DIM);
    int start = hcount > 5 ? hcount - 5 : 0;
    for (int i = start; i < hcount; i++) {
        char mbuf[12];
        snprintf(mbuf, 12, "%s-%s", history[i].from, history[i].to);
        mvaddstr(oy + 14 + i - start, ix, mbuf);
    }

    /* controls */
    attron(COLOR_PAIR(4) | A_DIM);
    mvaddstr(oy + 20, ix,
             _("Arrows: move  Enter: select/move",
               "Setas: mover  Enter: selecionar/mover"));
    mvaddstr(oy + 21, ix,
             _("Q: quit  U: toggle Unicode",
               "Q: sair  U: alterar Unicode"));
    attroff(COLOR_PAIR(4) | A_DIM);
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

    int play = 1;
    while (play) {
        init_board();

        while (1) {
            /* auto-check end */
            if (!game_over) check_game_end();

            erase();
            int oy = (LINES - 12) / 2;
            if (oy < 1) oy = 1;
            int ox = (COLS - 40) / 2;
            if (ox < 1) ox = 1;

            draw_board(ox, oy);
            draw_info(ox, oy);
            refresh();

            int ch = getch();
            if (ch == 'q') { play = 0; break; }
            if (ch == 'u' || ch == 'U') { use_unicode = !use_unicode; continue; }

            if (game_over) {
                if (ch == '\n' || ch == ' ') break;
                continue;
            }

            /* cursor movement */
            int nr = cur_r, nc = cur_c;
            if (ch == KEY_UP || ch == 'w') nr--;
            else if (ch == KEY_DOWN || ch == 's') nr++;
            else if (ch == KEY_LEFT || ch == 'a') nc--;
            else if (ch == KEY_RIGHT || ch == 'd') nc++;
            if (inb(nr, nc)) { cur_r = nr; cur_c = nc; }

            /* select / move */
            if (ch == '\n' || ch == ' ') {
                if (!selected) {
                    /* select a piece */
                    if (board[cur_r][cur_c].type != EMPTY
                        && board[cur_r][cur_c].color == turn) {
                        sel_r = cur_r; sel_c = cur_c;
                        selected = 1;
                    }
                } else {
                    /* try to move */
                    if (cur_r == sel_r && cur_c == sel_c) {
                        selected = 0; /* deselect */
                    } else if (is_valid_move(sel_r, sel_c, cur_r, cur_c)) {
                        make_move(sel_r, sel_c, cur_r, cur_c);
                        selected = 0;
                    } else {
                        /* try to select a different piece */
                        if (board[cur_r][cur_c].type != EMPTY
                            && board[cur_r][cur_c].color == turn) {
                            sel_r = cur_r; sel_c = cur_c;
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
