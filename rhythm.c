/******************************************************************************
 *                          RHYTHM SPIRE
 *     A 4-lane vertical scrolling rhythm game — feel the beat!
 ******************************************************************************/
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include "config.h"

#define LANES 4
#define MAX_NOTES 512
#define TARGET_ROW 14
#define WIN_PERFECT 3
#define WIN_GOOD 8
#define NOTES_PER_SONG 80
#define FRAMES_PER_BEAT 32
#define SPEED 0.45f
#define MAX_COMBO_DISPLAY 6

static const char *keys = "dfjk";

typedef struct { int lane, active; float trigger; } Note;
static Note notes[MAX_NOTES];
static int ncount;

static int score, combo, max_combo, total, perf, good, miss;
static float frame;
static int over, paused;
static int flash_perf, flash_good, flash_miss;

static int cw, ch, ox, oy, flash_len;

/* ───────── Generate beat map ───────── */
static void gen_notes(void) {
    ncount = 0;
    for (int i = 0; i < NOTES_PER_SONG && ncount < MAX_NOTES; i++) {
        float t = i * FRAMES_PER_BEAT;
        int lane = rand() % LANES;
        notes[ncount].lane = lane;
        notes[ncount].active = 1;
        notes[ncount].trigger = t;
        ncount++;

        /* occasional double */
        if (i < NOTES_PER_SONG - 1 && (rand() % 100) < 22 && ncount < MAX_NOTES) {
            int l2;
            do { l2 = rand() % LANES; } while (l2 == lane);
            notes[ncount].lane = l2;
            notes[ncount].active = 1;
            notes[ncount].trigger = t;
            ncount++;
        }

        /* occasional triplet (beat skip) */
        if (i < NOTES_PER_SONG - 2 && (rand() % 100) < 10 && ncount < MAX_NOTES) {
            notes[ncount].lane = rand() % LANES;
            notes[ncount].active = 1;
            notes[ncount].trigger = t + FRAMES_PER_BEAT * 0.5f;
            ncount++;
        }

        /* later levels: denser */
        if (i > 40 && (rand() % 100) < 20) i++;
        if (i > 60 && (rand() % 100) < 15) i++;
    }
}

/* ───────── Init ───────── */
static void init_game(void) {
    score = 0; combo = 0; max_combo = 0;
    total = 0; perf = 0; good = 0; miss = 0;
    frame = -TARGET_ROW / SPEED - 10;
    over = 0; paused = 0;
    flash_perf = 0; flash_good = 0; flash_miss = 0;
    flash_len = 10;
    gen_notes();
}

/* ───────── Hit lane ───────── */
static void hit_lane(int lane) {
    int best = -1;
    float best_dist = 999;
    for (int i = 0; i < ncount; i++) {
        if (!notes[i].active) continue;
        if (notes[i].lane != lane) continue;
        float dist = fabsf(notes[i].trigger - frame);
        if (dist < best_dist) { best = i; best_dist = dist; }
    }
    if (best < 0 || best_dist > WIN_GOOD) {
        combo = 0; flash_miss = flash_len; return;
    }
    notes[best].active = 0;
    total++;

    if (best_dist <= WIN_PERFECT) {
        combo++; score += 100 + combo * 5; perf++;
        flash_perf = flash_len;
    } else {
        combo++; score += 50 + combo * 2; good++;
        flash_good = flash_len;
    }
    if (combo > max_combo) max_combo = combo;
}

/* ───────── Update ───────── */
static void update(void) {
    if (paused) return;
    frame += 1.0f;

    /* auto-miss notes that passed */
    for (int i = 0; i < ncount; i++) {
        if (!notes[i].active) continue;
        if (notes[i].trigger < frame - WIN_GOOD) {
            notes[i].active = 0;
            combo = 0; total++; miss++;
            flash_miss = flash_len;
        }
    }

    if (flash_perf > 0) flash_perf--;
    if (flash_good > 0) flash_good--;
    if (flash_miss > 0) flash_miss--;

    /* check end */
    int any = 0;
    for (int i = 0; i < ncount; i++)
        if (notes[i].active) { any = 1; break; }
    if (!any && frame > NOTES_PER_SONG * FRAMES_PER_BEAT + 30)
        over = 1;
}

/* ───────── Note y position ───────── */
static float note_y(Note *n) {
    return TARGET_ROW - (n->trigger - frame) * SPEED;
}

/* ───────── Draw ───────── */
static void draw(void) {
    erase();

    int ls = ox + cw / 2 - LANES * 3 + 1;

    /* header */
    attron(COLOR_PAIR(2) | A_BOLD);
    mvaddstr(oy - 1, ox + cw / 2 - 7, _("RHYTHM SPIRE", "RITMO NA SPIRE"));
    attroff(COLOR_PAIR(2) | A_BOLD);

    char buf[80];
    snprintf(buf, 80, _("Score: %d  Combo: x%d", "Pontos: %d  Combo: x%d"),
             score, combo);
    attron(COLOR_PAIR(5));
    mvaddstr(oy - 1, ox + 2, buf);
    attroff(COLOR_PAIR(5));

    if (total > 0) {
        int pct = (perf + good / 2) * 100 / total;
        snprintf(buf, 80, _("Acc: %d%%", "Prec: %d%%"), pct);
        attron(COLOR_PAIR(6));
        mvaddstr(oy - 1, ox + cw - 12, buf);
        attroff(COLOR_PAIR(6));
    }

    /* Lane labels */
    for (int l = 0; l < LANES; l++) {
        int lx = ls + l * 6;
        attron(COLOR_PAIR(6) | A_BOLD);
        mvaddch(oy + 1, lx, keys[l] - 32);
        attroff(COLOR_PAIR(6) | A_BOLD);
    }

    /* target zone */
    attron(COLOR_PAIR(4) | A_BOLD);
    for (int l = 0; l < LANES; l++) {
        int lx = ls + l * 6;
        mvaddstr(oy + TARGET_ROW, lx - 1, ">>=<<");
    }
    attroff(COLOR_PAIR(4) | A_BOLD);

    /* lane lines */
    attron(COLOR_PAIR(4) | A_DIM);
    for (int l = 0; l <= LANES; l++) {
        int lx = ls + l * 6 - 3;
        for (int y = oy + 2; y < oy + TARGET_ROW; y++)
            mvaddch(y, lx, ':');
    }
    /* Center lanes */
    for (int l = 0; l < LANES; l++) {
        int lx = ls + l * 6;
        for (int y = oy + 2; y < oy + TARGET_ROW; y += 2)
            mvaddch(y, lx, '.');
    }
    attroff(COLOR_PAIR(4) | A_DIM);

    /* notes */
    for (int i = 0; i < ncount; i++) {
        if (!notes[i].active) continue;
        float y = note_y(&notes[i]);
        int ny = oy + (int)(y + 0.5f);
        if (ny < oy - 2 || ny > oy + TARGET_ROW + 3) continue;
        int nx = ls + notes[i].lane * 6;
        float dist = fabsf(y - TARGET_ROW);
        int col;
        if (dist <= WIN_PERFECT * SPEED) col = 2;
        else if (dist <= WIN_GOOD * SPEED) col = 5;
        else col = 3;
        attron(COLOR_PAIR(col) | A_BOLD);
        mvaddch(ny, nx, '#');
        attroff(COLOR_PAIR(col) | A_BOLD);
    }

    /* feedback */
    if (flash_perf > 0) {
        attron(COLOR_PAIR(2) | A_BOLD);
        mvaddstr(oy + TARGET_ROW + 3, ox + cw / 2 - 4,
                 _("PERFECT!", "PERFEITO!"));
        attroff(COLOR_PAIR(2) | A_BOLD);
    } else if (flash_good > 0) {
        attron(COLOR_PAIR(5) | A_BOLD);
        mvaddstr(oy + TARGET_ROW + 3, ox + cw / 2 - 2, _("GOOD", "BOM"));
        attroff(COLOR_PAIR(5) | A_BOLD);
    } else if (flash_miss > 0) {
        attron(COLOR_PAIR(3) | A_BOLD);
        mvaddstr(oy + TARGET_ROW + 3, ox + cw / 2 - 2, _("MISS", "ERROU"));
        attroff(COLOR_PAIR(3) | A_BOLD);
    }

    /* combo indicator */
    if (combo >= 5) {
        int col = combo >= 20 ? 2 : combo >= 10 ? 5 : 6;
        attron(COLOR_PAIR(col) | A_BOLD);
        char cbuf[16];
        snprintf(cbuf, 16, "x%d!", combo);
        mvaddstr(oy + TARGET_ROW + 5, ox + cw / 2 - 2, cbuf);
        attroff(COLOR_PAIR(col) | A_BOLD);
    }

    if (paused) {
        attron(COLOR_PAIR(2) | A_BOLD);
        mvaddstr(oy + ch / 2, ox + cw / 2 - 3, _("PAUSED", "PAUSADO"));
        attroff(COLOR_PAIR(2) | A_BOLD);
    }

    /* bottom bar */
    attron(COLOR_PAIR(4) | A_DIM);
    if (!over)
        mvaddstr(oy + ch, ox + 2,
                 _("D F J K: hit  P: pause  Q: quit",
                   "D F J K: nota  P: pausa  Q: sair"));
    else
        mvaddstr(oy + ch, ox + 2,
                 _("Enter: play again  Q: quit",
                   "Enter: jogar novamente  Q: sair"));
    attroff(COLOR_PAIR(4) | A_DIM);

    /* results */
    if (over) {
        int ry = oy + ch / 2 - 4;
        attron(COLOR_PAIR(2) | A_BOLD);
        mvaddstr(ry, ox + cw / 2 - 6, _("CLEAR!", "COMPLETO!"));
        attroff(COLOR_PAIR(2) | A_BOLD);
        ry += 2;
        attron(COLOR_PAIR(5));
        snprintf(buf, 80, _("Score: %d  Max Combo: %d",
                             "Pontos: %d  Combo Max: %d"), score, max_combo);
        mvaddstr(ry, ox + cw / 2 - 14, buf);
        ry += 1;
        int pct = total > 0 ? (perf + good / 2) * 100 / total : 0;
        snprintf(buf, 80, _("P:%d  G:%d  M:%d  Acc:%d%%",
                             "P:%d  B:%d  E:%d  Prec:%d%%"),
                 perf, good, miss, pct);
        mvaddstr(ry, ox + cw / 2 - 14, buf);
        attroff(COLOR_PAIR(5));
    }

    refresh();
}

/* ───────── Main ───────── */
int main(void) {
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

    int tr, tc;
    getmaxyx(stdscr, tr, tc);
    ch = 20; cw = 40;
    oy = (tr - ch) / 2; if (oy < 1) oy = 1;
    ox = (tc - cw) / 2; if (ox < 1) ox = 1;

    int play = 1;
    while (play) {
        init_game();
        while (1) {
            int c = getch();
            if (c == 'q') { play = 0; break; }

            if (over) {
                if (c == '\n' || c == ' ') break;
                continue;
            }

            if (c == 'p' || c == 'P') { paused = !paused; continue; }
            if (paused) continue;

            if (c == 'd' || c == 'D') hit_lane(0);
            else if (c == 'f' || c == 'F') hit_lane(1);
            else if (c == 'j' || c == 'J') hit_lane(2);
            else if (c == 'k' || c == 'K') hit_lane(3);

            update();
            draw();
            usleep(16000);
        }
        if (!play) break;
    }

    endwin();
    return 0;
}
