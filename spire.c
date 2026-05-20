/******************************************************************************
 *                            SPIRE ASCENT
 *         A deck-building roguelike — climb the Spire!
 *         Featuring a procedurally generated map and relics.
 ******************************************************************************/
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "config.h"

/* ───────── Constants ───────── */
#define MAX_DECK 30
#define MAX_HAND 10
#define MAX_DISCARD 30
#define MAX_RELICS 8
#define MAP_ROWS 7
#define MAP_COLS 3

/* ───────── Node types ───────── */
enum { NODE_MONSTER, NODE_ELITE, NODE_REST, NODE_SHOP, NODE_TREASURE, NODE_BOSS };

/* ───────── Relic IDs ───────── */
enum { RELIC_BURNING_BLADE, RELIC_IRON_SHIELD, RELIC_HEAL_AMULET,
       RELIC_ENERGY_CRYSTAL, RELIC_LUCKY_CLOVER, RELIC_VOODOO_DOLL,
       RELIC_WAR_PAINT, RELIC_BLOOD_VIAL, RELIC_COUNT };

/* ───────── Card ───────── */
typedef struct {
    char name[16];
    int cost, dmg, block, heal, draw;
} Card;

static const Card card_defs[] = {
    { "Strike",     1, 6,  0, 0, 0 },
    { "Defend",     1, 0,  5, 0, 0 },
    { "Bash",       2, 9,  0, 0, 0 },
    { "Shield Bash",1, 4,  4, 0, 0 },
    { "Heavy",      3, 20, 0, 0, 0 },
    { "Slash",      1, 8,  0, 0, 0 },
    { "Reinforce",  1, 0,  8, 0, 0 },
    { "Bandage",    1, 0,  0, 8, 0 },
    { "Cleave",     2, 12, 0, 0, 0 },
    { "Fortify",    2, 0,  12, 0, 0 },
    { "Adrenaline", 0, 0,  0, 0, 2 },
};
#define CARD_COUNT ((int)(sizeof(card_defs)/sizeof(card_defs[0])))

/* ───────── Relic data ───────── */
static const char *relic_names[RELIC_COUNT] = {
    "Burning Blade", "Iron Shield", "Healing Amulet",
    "Energy Crystal", "Lucky Clover", "Voodoo Doll",
    "War Paint", "Blood Vial"
};
static const char *relic_desc[RELIC_COUNT] = {
    "Attacks deal +2 damage",
    "Blocks give +2 block",
    "Heal 5 HP after each combat",
    "+1 max energy",
    "Draw 1 extra card each turn",
    "Enemies start with -3 HP",
    "Start each combat with 5 block",
    "+15 max HP"
};

/* ───────── Map node ───────── */
typedef struct {
    int type;
    int cleared;
} MapNode;

/* ───────── Game state ───────── */
static Card deck[MAX_DECK];        static int deck_sz;
static Card hand[MAX_HAND];        static int hand_sz;
static Card discard[MAX_DISCARD];  static int discard_sz;
static Card *draw_pile[MAX_DECK];  static int draw_sz;
static int relics[MAX_RELICS];     static int relic_sz;
static int hp, max_hp, energy, max_energy, block;
static int gold, turn, over, won;
static int floor_idx, map_col;
static MapNode map[MAP_ROWS][MAP_COLS];

typedef struct {
    char name[20];
    int hp, max_hp, dmg;
    int intent_dmg;
} Enemy;
static Enemy enemies[2];
static int enemy_count;

/* ───────── Helpers ───────── */
static int rng(int n) { return rand() % n; }

static void shuffle(int *arr, int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = rng(i + 1);
        int t = arr[i]; arr[i] = arr[j]; arr[j] = t;
    }
}

static int has_relic(int id) {
    for (int i = 0; i < relic_sz; i++)
        if (relics[i] == id) return 1;
    return 0;
}

/* ───────── Deck management ───────── */
static void refill_draw(void) {
    int idx[MAX_DISCARD], n = discard_sz;
    for (int i = 0; i < n; i++) idx[i] = i;
    shuffle(idx, n);
    draw_sz = 0;
    for (int i = 0; i < n; i++)
        draw_pile[draw_sz++] = &discard[idx[i]];
    discard_sz = 0;
}

static void draw_cards(int n) {
    for (int i = 0; i < n && hand_sz < MAX_HAND; i++) {
        if (draw_sz == 0) refill_draw();
        if (draw_sz == 0) break;
        hand[hand_sz++] = *draw_pile[--draw_sz];
    }
}

static void discard_hand(void) {
    for (int i = 0; i < hand_sz && discard_sz < MAX_DISCARD; i++)
        discard[discard_sz++] = hand[i];
    hand_sz = 0;
}

static void add_card(int def_idx) {
    if (deck_sz < MAX_DECK)
        deck[deck_sz++] = card_defs[def_idx];
}

static void add_relic(int id) {
    if (relic_sz < MAX_RELICS)
        relics[relic_sz++] = id;
}

/* ───────── Map generation ───────── */
static int node_symbol(int type) {
    switch (type) {
        case NODE_MONSTER:  return 'M';
        case NODE_ELITE:    return 'E';
        case NODE_REST:     return 'R';
        case NODE_SHOP:     return '$';
        case NODE_TREASURE: return 'T';
        case NODE_BOSS:     return 'B';
        default:            return '?';
    }
}

static void gen_map(void) {
    for (int r = 0; r < MAP_ROWS; r++) {
        for (int c = 0; c < MAP_COLS; c++) {
            map[r][c].cleared = 0;
            if (r == MAP_ROWS - 1) {
                map[r][c].type = (c == 1) ? NODE_BOSS : -1;
                continue;
            }
            int roll = rng(100);
            if (roll < 35)      map[r][c].type = NODE_MONSTER;
            else if (roll < 50) map[r][c].type = NODE_REST;
            else if (roll < 65) map[r][c].type = NODE_SHOP;
            else if (roll < 80) map[r][c].type = NODE_TREASURE;
            else                map[r][c].type = NODE_ELITE;
            if (r <= 1 && map[r][c].type == NODE_ELITE) map[r][c].type = NODE_MONSTER;
        }
    }
    map[MAP_ROWS-1][1].type = NODE_BOSS;
}

static int is_valid_move(int r, int c) {
    if (r < 0 || r >= MAP_ROWS || c < 0 || c >= MAP_COLS) return 0;
    return map[r][c].type >= 0;
}

static int can_reach(int r, int c) {
    if (r == 0) return is_valid_move(0, c);
    for (int dc = -1; dc <= 1; dc++) {
        int pc = map_col + dc;
        if (is_valid_move(r - 1, pc) && map[r - 1][pc].cleared)
            if (pc == c || pc + 1 == c || pc - 1 == c)
                return 1;
    }
    return 0;
}

/* ───────── Enemy generation ───────── */
static void make_enemy(int idx, int is_elite) {
    int f = floor_idx + 1;
    char *names[] = {"Slime", "Zombie", "Golem", "Imp", "Skeleton"};
    int n = rng(5);
    snprintf(enemies[idx].name, 20, "%s", names[n]);

    int voodoo = has_relic(RELIC_VOODOO_DOLL) ? 3 : 0;
    if (is_elite) {
        enemies[idx].max_hp = 30 + f * 5 - voodoo;
        enemies[idx].dmg = 7 + f * 2;
    } else {
        enemies[idx].max_hp = 14 + f * 3 - voodoo;
        enemies[idx].dmg = 4 + f;
    }
    if (enemies[idx].max_hp < 1) enemies[idx].max_hp = 1;
    enemies[idx].hp = enemies[idx].max_hp;
}

/* ───────── Enemy intents ───────── */
static void set_intents(void) {
    for (int i = 0; i < enemy_count; i++) {
        if (enemies[i].hp <= 0) continue;
        int dmg = enemies[i].dmg + (rng(5) - 2);
        if (dmg < 1) dmg = 1;
        enemies[i].intent_dmg = dmg;
    }
}

/* ───────── Combat ───────── */
static void start_combat(int is_elite) {
    enemy_count = (is_elite || floor_idx >= 4) ? 2 : 1;
    if (enemy_count > 2) enemy_count = 2;
    for (int i = 0; i < enemy_count; i++)
        make_enemy(i, is_elite);
    energy = max_energy;
    block = has_relic(RELIC_WAR_PAINT) ? 5 : 0;
    discard_hand();
    draw_sz = 0;
    for (int i = 0; i < deck_sz; i++) draw_pile[draw_sz++] = &deck[i];
    int order[MAX_DECK];
    for (int i = 0; i < draw_sz; i++) order[i] = i;
    shuffle(order, draw_sz);
    Card *tmp[MAX_DECK];
    for (int i = 0; i < draw_sz; i++) tmp[i] = draw_pile[order[i]];
    for (int i = 0; i < draw_sz; i++) draw_pile[i] = tmp[i];
    discard_sz = 0;
    hand_sz = 0;
    draw_cards(5 + (has_relic(RELIC_LUCKY_CLOVER) ? 1 : 0));
    set_intents();
}

static void enemy_turn(void) {
    for (int i = 0; i < enemy_count; i++) {
        if (enemies[i].hp <= 0) continue;
        int dmg = enemies[i].intent_dmg;
        if (dmg > block) {
            hp -= (dmg - block);
            block = 0;
        } else {
            block -= dmg;
        }
        if (hp <= 0) { over = 1; return; }
    }
    block = 0;
    energy = max_energy;
    discard_hand();
    draw_cards(5 + (has_relic(RELIC_LUCKY_CLOVER) ? 1 : 0));
    set_intents();
}

static void play_card(int idx) {
    if (idx >= hand_sz) return;
    Card *c = &hand[idx];
    if (c->cost > energy) return;

    int target = -1;
    for (int i = 0; i < enemy_count; i++)
        if (enemies[i].hp > 0) { target = i; break; }
    if (target < 0) return;

    energy -= c->cost;

    int bonus_dmg = has_relic(RELIC_BURNING_BLADE) ? 2 : 0;
    int bonus_blk = has_relic(RELIC_IRON_SHIELD) ? 2 : 0;

    block += c->block + bonus_blk;
    enemies[target].hp -= c->dmg + bonus_dmg;
    hp += c->heal;
    if (hp > max_hp) hp = max_hp;

    if (c->draw > 0) draw_cards(c->draw);

    if (discard_sz < MAX_DISCARD)
        discard[discard_sz++] = *c;

    for (int i = idx; i < hand_sz - 1; i++)
        hand[i] = hand[i + 1];
    hand_sz--;
}

static void end_turn(void) {
    turn++;
    enemy_turn();
}

/* ───────── Combat screen ───────── */
static int run_combat(int oy, int ox, int is_elite) {
    start_combat(is_elite);
    int ch;
    while (1) {
        int all_dead = 1;
        for (int i = 0; i < enemy_count; i++)
            if (enemies[i].hp > 0) all_dead = 0;
        if (all_dead) break;
        if (over) break;

        ch = getch();
        if (ch == 'q') { over = 1; return 0; }

        if (ch >= '1' && ch <= '9') {
            int idx = ch - '1';
            if (idx < hand_sz) play_card(idx);
        }
        if (ch == '\n' || ch == ' ') end_turn();

        erase();
        attron(COLOR_PAIR(2) | A_BOLD);
        char hdr[48];
        snprintf(hdr, 48, _("Floor %d/%d", "Andar %d/%d"), floor_idx + 1, MAP_ROWS - 1);
        mvaddstr(oy, ox + 18, hdr);
        attroff(COLOR_PAIR(2) | A_BOLD);

        for (int i = 0; i < enemy_count; i++) {
            if (enemies[i].hp <= 0) continue;
            int ex = ox + 4 + i * 22;
            int ey = oy + 2;
            attron(COLOR_PAIR(3) | A_BOLD);
            mvaddstr(ey, ex, enemies[i].name);
            attroff(COLOR_PAIR(3) | A_BOLD);
            char intent_str[16];
            snprintf(intent_str, 16, " \xe2\x9a\x94%d", enemies[i].intent_dmg);
            attron(COLOR_PAIR(1) | A_BOLD);
            mvaddstr(ey, ex + strlen(enemies[i].name) + 1, intent_str);
            attroff(COLOR_PAIR(1) | A_BOLD);
            int bar_w = 14;
            int filled = enemies[i].hp * bar_w / (enemies[i].max_hp > 0 ? enemies[i].max_hp : 1);
            if (filled < 0) filled = 0;
            if (filled > bar_w) filled = bar_w;
            attron(COLOR_PAIR(5));
            mvaddch(ey + 1, ex, '[');
            attroff(COLOR_PAIR(5));
            for (int b = 0; b < bar_w; b++) {
                if (b < filled) { attron(COLOR_PAIR(3) | A_BOLD); mvaddch(ey + 1, ex + 1 + b, '#'); attroff(COLOR_PAIR(3) | A_BOLD); }
                else { attron(COLOR_PAIR(4) | A_DIM); mvaddch(ey + 1, ex + 1 + b, '.'); attroff(COLOR_PAIR(4) | A_DIM); }
            }
            attron(COLOR_PAIR(5));
            mvaddch(ey + 1, ex + 1 + bar_w, ']');
            attroff(COLOR_PAIR(5));
            char hp_str[16];
            snprintf(hp_str, 16, "%d/%d", enemies[i].hp, enemies[i].max_hp);
            attron(COLOR_PAIR(6));
            mvaddstr(ey + 2, ex + 3, hp_str);
            attroff(COLOR_PAIR(6));
        }

        attron(COLOR_PAIR(4) | A_DIM);
        for (int x = 0; x < 48; x++) mvaddch(oy + 5, ox + 1 + x, '-');
        attroff(COLOR_PAIR(4) | A_DIM);

        int py = oy + 7;
        attron(COLOR_PAIR(2) | A_BOLD);
        mvaddstr(py, ox + 2, _("YOU", "VOCE"));
        attroff(COLOR_PAIR(2) | A_BOLD);
        char pbuf[64];
        snprintf(pbuf, 64, "HP: %d/%d  \xe2\x9a\xa1%d/%d  \xf0\x9f\x9b\xa1%d",
                 hp, max_hp, energy, max_energy, block);
        attron(COLOR_PAIR(5));
        mvaddstr(py + 1, ox + 2, pbuf);
        attroff(COLOR_PAIR(5));

        int hp_w = 20;
        int hp_filled = hp * hp_w / (max_hp > 0 ? max_hp : 1);
        if (hp_filled < 0) hp_filled = 0;
        if (hp_filled > hp_w) hp_filled = hp_w;
        mvaddch(py + 2, ox + 2, '[');
        for (int b = 0; b < hp_w; b++) {
            attron((b < hp_filled) ? (COLOR_PAIR(1) | A_BOLD) : (COLOR_PAIR(4) | A_DIM));
            mvaddch(py + 2, ox + 3 + b, b < hp_filled ? '#' : '.');
            attroff((b < hp_filled) ? (COLOR_PAIR(1) | A_BOLD) : (COLOR_PAIR(4) | A_DIM));
        }
        mvaddch(py + 2, ox + 3 + hp_w, ']');

        int hy = py + 4;
        attron(COLOR_PAIR(6) | A_DIM);
        mvaddstr(hy, ox + 2, _("Hand:", "Mao:"));
        attroff(COLOR_PAIR(6) | A_DIM);

        for (int i = 0; i < hand_sz; i++) {
            int cy = hy + 1 + i;
            int can_play = (hand[i].cost <= energy);
            attron(can_play ? COLOR_PAIR(2) : COLOR_PAIR(4) | A_DIM);
            mvprintw(cy, ox + 4, "%d. %s (\xe2\x9a\xa1%d)", i + 1, hand[i].name, hand[i].cost);
            int pos = 24;
            char stats[16];
            if (hand[i].dmg > 0) {
                snprintf(stats, 16, "\xe2\x9a\x94%d", hand[i].dmg);
                mvaddstr(cy, ox + pos, stats); pos += 8;
            }
            if (hand[i].block > 0) {
                snprintf(stats, 16, "\xf0\x9f\x9b\xa1%d", hand[i].block);
                mvaddstr(cy, ox + pos, stats); pos += 8;
            }
            if (hand[i].heal > 0) {
                snprintf(stats, 16, "+%dHP", hand[i].heal);
                mvaddstr(cy, ox + pos, stats); pos += 8;
            }
            if (hand[i].draw > 0) {
                mvaddstr(cy, ox + pos, _("draw", "compra"));
            }
            attroff(can_play ? COLOR_PAIR(2) : COLOR_PAIR(4) | A_DIM);
        }

        int by = oy + 20;
        attron(COLOR_PAIR(6) | A_DIM);
        mvaddstr(by, ox + 2,
                 _("1-9: card | Enter/Space: end turn | Q: quit",
                   "1-9: carta | Enter/Espaco: turno | Q: sair"));
        attroff(COLOR_PAIR(6) | A_DIM);
        refresh();
        usleep(30000);
    }
    return !over;
}

/* ───────── Reward / Card choice ───────── */
static void card_reward(int oy, int ox) {
    erase();
    attron(COLOR_PAIR(2) | A_BOLD);
    mvaddstr(oy + 2, ox + 6, _("VICTORY!", "VITORIA!"));
    attroff(COLOR_PAIR(2) | A_BOLD);
    attron(COLOR_PAIR(5));
    mvaddstr(oy + 4, ox + 2, _("Choose a card to add to your deck:",
                                "Escolha uma carta:"));
    attroff(COLOR_PAIR(5));

    int choices[3];
    for (int i = 0; i < 3; i++) {
        int r;
        do { r = rng(CARD_COUNT); } while (r == 0 || r == 1);
        choices[i] = r;
    }
    for (int i = 0; i < 3; i++) {
        int cy = oy + 6 + i;
        attron(A_REVERSE);
        mvaddstr(cy, ox + 2, "  ");
        attroff(A_REVERSE);
        mvaddstr(cy, ox + 4, card_defs[choices[i]].name);
        char info[40];
        snprintf(info, 40, "  \xe2\x9a\xa1%d  \xe2\x9a\x94%d  \xf0\x9f\x9b\xa1%d",
                 card_defs[choices[i]].cost, card_defs[choices[i]].dmg,
                 card_defs[choices[i]].block);
        mvaddstr(cy, ox + 20, info);
    }
    attron(COLOR_PAIR(6) | A_DIM);
    mvaddstr(oy + 10, ox + 2,
             _("1/2/3: choose | Enter: skip", "1/2/3: escolher | Enter: pular"));
    attroff(COLOR_PAIR(6) | A_DIM);
    refresh();
    nodelay(stdscr, FALSE);
    while (1) {
        int c = getch();
        if (c >= '1' && c <= '3') { add_card(choices[c - '1']); break; }
        if (c == '\n') break;
    }
    nodelay(stdscr, TRUE);
}

static void relic_reward(int oy, int ox) {
    erase();
    attron(COLOR_PAIR(2) | A_BOLD);
    mvaddstr(oy + 3, ox + 8,
             _("You found a relic!", "Voce encontrou uma reliquia!"));
    attroff(COLOR_PAIR(2) | A_BOLD);

    int r1 = rng(RELIC_COUNT), r2;
    do { r2 = rng(RELIC_COUNT); } while (r2 == r1);

    for (int i = 0; i < 2; i++) {
        int id = (i == 0) ? r1 : r2;
        int cy = oy + 6 + i * 3;
        attron(COLOR_PAIR(6) | A_BOLD);
        mvaddstr(cy, ox + 4, (i == 0) ? "1." : "2.");
        mvaddstr(cy, ox + 7, relic_names[id]);
        attroff(COLOR_PAIR(6) | A_BOLD);
        attron(COLOR_PAIR(5) | A_DIM);
        mvaddstr(cy + 1, ox + 7, relic_desc[id]);
        attroff(COLOR_PAIR(5) | A_DIM);
    }
    attron(COLOR_PAIR(6) | A_DIM);
    mvaddstr(oy + 14, ox + 2,
             _("1/2: choose relic | Enter: skip",
               "1/2: escolher | Enter: pular"));
    attroff(COLOR_PAIR(6) | A_DIM);
    refresh();
    nodelay(stdscr, FALSE);
    while (1) {
        int c = getch();
        if (c == '1') { add_relic(r1); break; }
        if (c == '2') { add_relic(r2); break; }
        if (c == '\n') break;
    }
    nodelay(stdscr, TRUE);
}

/* ───────── Rest ───────── */
static void do_rest(int oy, int ox) {
    int heal = max_hp * 30 / 100;
    if (heal < 5) heal = 5;
    hp += heal;
    if (hp > max_hp) hp = max_hp;
    erase();
    attron(COLOR_PAIR(1) | A_BOLD);
    mvaddstr(oy + 5, ox + 10, _("REST", "DESCANSO"));
    attroff(COLOR_PAIR(1) | A_BOLD);
    attron(COLOR_PAIR(5));
    char buf[48];
    snprintf(buf, 48, _("Healed %d HP!", "Recuperou %d HP!"), heal);
    mvaddstr(oy + 8, ox + 8, buf);
    attroff(COLOR_PAIR(5));
    attron(COLOR_PAIR(6) | A_DIM);
    mvaddstr(oy + 12, ox + 4,
             _("Press any key to continue...", "Pressione qualquer tecla..."));
    attroff(COLOR_PAIR(6) | A_DIM);
    refresh();
    nodelay(stdscr, FALSE);
    getch();
    nodelay(stdscr, TRUE);
}

/* ───────── Shop ───────── */
static void do_shop(int oy, int ox) {
    int cards[3], relics_l[2];
    for (int i = 0; i < 3; i++) {
        int r;
        do { r = rng(CARD_COUNT); } while (r == 0 || r == 1);
        cards[i] = r;
    }
    for (int i = 0; i < 2; i++) relics_l[i] = rng(RELIC_COUNT);

    while (1) {
        erase();
        attron(COLOR_PAIR(2) | A_BOLD);
        mvaddstr(oy + 1, ox + 16, _("SHOP", "LOJA"));
        attroff(COLOR_PAIR(2) | A_BOLD);
        char gp[32];
        snprintf(gp, 32, _("Gold: %d", "Ouro: %d"), gold);
        attron(COLOR_PAIR(5));
        mvaddstr(oy + 2, ox + 16, gp);
        attroff(COLOR_PAIR(5));
        mvaddstr(oy + 4, ox + 2, _("Cards (10g):", "Cartas (10g):"));
        for (int i = 0; i < 3; i++) {
            int cy = oy + 5 + i;
            mvprintw(cy, ox + 4, "%d. %s \xe2\x9a\xa1%d \xe2\x9a\x94%d",
                     i + 1, card_defs[cards[i]].name,
                     card_defs[cards[i]].cost, card_defs[cards[i]].dmg);
        }
        mvaddstr(oy + 9, ox + 2, _("Relics (20g):", "Reliquias (20g):"));
        for (int i = 0; i < 2; i++) {
            int cy = oy + 10 + i;
            mvprintw(cy, ox + 4, "%d. %s", i + 4, relic_names[relics_l[i]]);
            attron(COLOR_PAIR(4) | A_DIM);
            mvaddstr(cy + 1, ox + 6, relic_desc[relics_l[i]]);
            attroff(COLOR_PAIR(4) | A_DIM);
        }
        int by = oy + 14;
        attron(COLOR_PAIR(6) | A_DIM);
        mvaddstr(by, ox + 2,
                 _("1-3: buy card | 4-5: buy relic | Enter: leave",
                   "1-3: comprar carta | 4-5: comprar relic | Enter: sair"));
        attroff(COLOR_PAIR(6) | A_DIM);
        refresh();
        nodelay(stdscr, FALSE);
        int c = getch();
        nodelay(stdscr, TRUE);
        if (c == '\n') break;
        if (c >= '1' && c <= '3') {
            int idx = c - '1';
            if (gold >= 10) { gold -= 10; add_card(cards[idx]); }
        }
        if (c >= '4' && c <= '5') {
            int idx = c - '4';
            if (gold >= 20) { gold -= 20; add_relic(relics_l[idx]); }
        }
    }
}

/* ───────── Map screen ───────── */
static int map_screen(int oy, int ox) {
    while (1) {
        erase();
        attron(COLOR_PAIR(2) | A_BOLD);
        mvaddstr(oy, ox + 14, _("THE SPIRE", "A SPIRE"));
        attroff(COLOR_PAIR(2) | A_BOLD);

        /* Legend */
        attron(COLOR_PAIR(6) | A_DIM);
        mvaddstr(oy + 1, ox + 2,
                 _("M:Monster  E:Elite  R:Rest  $:Shop  T:Treas  B:Boss",
                   "M:Monstro  E:Elite  R:Desc  $:Loja  T:Tesou  B:Chefe"));
        attroff(COLOR_PAIR(6) | A_DIM);

        /* HP/relics status */
        char st[64];
        snprintf(st, 64, "HP:%d/%d  \xe2\x9a\xa1%d  G:%d  Rel:%d",
                 hp, max_hp, max_energy, gold, relic_sz);
        attron(COLOR_PAIR(5));
        mvaddstr(oy + 2, ox + 8, st);
        attroff(COLOR_PAIR(5));

        /* Grid – box drawing */
        int grid_oy = oy + 4;
        int cell_w = 5;
        int grid_ox = ox + 6;

        for (int r = 0; r < MAP_ROWS; r++) {
            /* Row of cells */
            for (int c = 0; c < MAP_COLS; c++) {
                int x = grid_ox + c * (cell_w + 1);
                int y = grid_oy + r * 2;
                int selected = (r == floor_idx && c == map_col);
                int reachable = can_reach(r, c);
                int is_valid = is_valid_move(r, c);

                /* Background for selected node */
                if (selected) {
                    attron(A_REVERSE);
                    for (int w = 0; w < cell_w; w++)
                        mvaddch(y, x + w, ' ');
                    attroff(A_REVERSE);
                } else if (r == 0 || (r > 0 && r <= floor_idx)) {
                    /* Already visited row or current row */
                }

                if (!is_valid) {
                    if (!selected) {
                        attron(COLOR_PAIR(4) | A_DIM);
                        for (int w = 0; w < cell_w; w++)
                            mvaddch(y, x + w, '.');
                        attroff(COLOR_PAIR(4) | A_DIM);
                    }
                    continue;
                }

                int sym = node_symbol(map[r][c].type);
                int color;
                int bold = 0;
                switch (map[r][c].type) {
                    case NODE_MONSTER:  color = 3; break;
                    case NODE_ELITE:    color = 3; bold = A_BOLD; break;
                    case NODE_REST:     color = 1; break;
                    case NODE_SHOP:     color = 2; break;
                    case NODE_TREASURE: color = 6; bold = A_BOLD; break;
                    case NODE_BOSS:     color = 3; bold = A_BOLD | A_BLINK; break;
                    default:            color = 4; break;
                }

                if (map[r][c].cleared)
                    attron(COLOR_PAIR(4) | A_DIM);
                else if (reachable || r == 0)
                    attron(COLOR_PAIR(color) | bold);
                else
                    attron(COLOR_PAIR(4) | A_DIM);

                mvaddch(y, x + cell_w / 2, sym);

                if (map[r][c].cleared)
                    attroff(COLOR_PAIR(4) | A_DIM);
                else
                    attroff(COLOR_PAIR(color) | bold);

                /* Cleared mark */
                if (map[r][c].cleared) {
                    attron(COLOR_PAIR(1));
                    mvaddch(y, x + cell_w - 2, 'V');
                    attroff(COLOR_PAIR(1));
                }
            }
            /* Connection lines between rows */
            if (r < MAP_ROWS - 1) {
                for (int c = 0; c < MAP_COLS; c++) {
                    if (!is_valid_move(r, c)) continue;
                    int x = grid_ox + c * (cell_w + 1) + cell_w / 2;
                    int y = grid_oy + r * 2 + 1;
                    attron(COLOR_PAIR(4) | A_DIM);
                    mvaddch(y, x, '|');
                    attroff(COLOR_PAIR(4) | A_DIM);
                }
            }
        }

        /* Right-side info for selected node */
        int info_y = grid_oy;
        int info_x = grid_ox + MAP_COLS * (cell_w + 1) + 4;
        int t = map[floor_idx][map_col].type;
        if (t >= 0) {
            attron(COLOR_PAIR(6) | A_BOLD);
            mvaddstr(info_y, info_x, _("Selected:", "Selecionado:"));
            attroff(COLOR_PAIR(6) | A_BOLD);
            info_y++;
            const char *names[] = {
                _("Monster", "Monstro"),
                _("Elite", "Elite"),
                _("Rest Site", "Descanso"),
                _("Shop", "Loja"),
                _("Treasure", "Tesouro"),
                _("Boss", "Chefe")
            };
            attron(COLOR_PAIR(5));
            mvaddstr(info_y, info_x, names[t]);
            attroff(COLOR_PAIR(5));
            info_y += 2;
            if (t == NODE_MONSTER || t == NODE_ELITE) {
                attron(COLOR_PAIR(3));
                mvaddstr(info_y, info_x, _("Combat awaits!", "Batalha te espera!"));
                attroff(COLOR_PAIR(3));
            } else if (t == NODE_REST) {
                attron(COLOR_PAIR(1));
                mvaddstr(info_y, info_x, _("Heal 30% HP", "Recupera 30% HP"));
                attroff(COLOR_PAIR(1));
            } else if (t == NODE_SHOP) {
                attron(COLOR_PAIR(2));
                mvaddstr(info_y, info_x, _("Buy cards & relics", "Compre cartas e reliquias"));
                attroff(COLOR_PAIR(2));
            } else if (t == NODE_TREASURE) {
                attron(COLOR_PAIR(6));
                mvaddstr(info_y, info_x, _("Free relic!", "Reliquia gratis!"));
                attroff(COLOR_PAIR(6));
            } else if (t == NODE_BOSS) {
                attron(COLOR_PAIR(3) | A_BOLD);
                mvaddstr(info_y, info_x, _("BOSS FIGHT!", "BATALHA CHEFE!"));
                attroff(COLOR_PAIR(3) | A_BOLD);
            }
        }

        /* Relics display */
        int rel_y = grid_oy + MAP_ROWS * 2 + 1;
        attron(COLOR_PAIR(6) | A_BOLD);
        mvaddstr(rel_y, ox + 2, _("Relics:", "Reliquias:"));
        attroff(COLOR_PAIR(6) | A_BOLD);
        if (relic_sz == 0) {
            attron(COLOR_PAIR(4) | A_DIM);
            mvaddstr(rel_y + 1, ox + 4, _("(none)", "(nenhuma)"));
            attroff(COLOR_PAIR(4) | A_DIM);
        } else {
            for (int i = 0; i < relic_sz; i++) {
                mvaddstr(rel_y + 1 + i, ox + 4, relic_names[relics[i]]);
            }
        }

        /* Controls */
        int by2 = rel_y + 10;
        attron(COLOR_PAIR(6) | A_DIM);
        mvaddstr(by2, ox + 2,
                 _("\xe2\x86\x90\xe2\x86\x92 or 1/2/3: move  | Enter: enter node | Q: quit",
                   "<- ou 1/2/3: mover  | Enter: entrar  | Q: sair"));
        attroff(COLOR_PAIR(6) | A_DIM);
        refresh();

        int ch = getch();
        if (ch == 'q') return 0;

        /* Column navigation */
        int new_col = map_col;
        if (ch == KEY_LEFT || ch == 'a') new_col--;
        else if (ch == KEY_RIGHT || ch == 'd') new_col++;
        else if (ch >= '1' && ch <= '3') new_col = ch - '1';

        if (new_col >= 0 && new_col < MAP_COLS && new_col != map_col) {
            if (floor_idx == 0 || can_reach(floor_idx, new_col)) {
                map_col = new_col;
            }
        }

        if (ch == '\n') {
            if (floor_idx == 0 || can_reach(floor_idx, map_col)) {
                if (is_valid_move(floor_idx, map_col) && !map[floor_idx][map_col].cleared) {
                    map[floor_idx][map_col].cleared = 1;
                    return 1;
                }
            }
        }
    }
}

/* ───────── Game initialization ───────── */
static void init_game(void) {
    max_hp = has_relic(RELIC_BLOOD_VIAL) ? 85 : 70;
    hp = max_hp;
    max_energy = 3 + (has_relic(RELIC_ENERGY_CRYSTAL) ? 1 : 0);
    gold = 0;
    floor_idx = 0;
    map_col = 1;
    turn = 0;
    over = 0;
    won = 0;
    deck_sz = 0;
    hand_sz = 0;
    discard_sz = 0;
    draw_sz = 0;
    relic_sz = 0;

    for (int i = 0; i < 5; i++) add_card(0);
    for (int i = 0; i < 4; i++) add_card(1);
    add_card(2);

    for (int i = 0; i < deck_sz; i++) draw_pile[i] = &deck[i];
    draw_sz = deck_sz;
    int order[MAX_DECK];
    for (int i = 0; i < draw_sz; i++) order[i] = i;
    shuffle(order, draw_sz);
    Card *tmp[MAX_DECK];
    for (int i = 0; i < draw_sz; i++) tmp[i] = draw_pile[order[i]];
    for (int i = 0; i < draw_sz; i++) draw_pile[i] = tmp[i];

    gen_map();
}

/* ───────── Process a node ───────── */
static void process_node(int oy, int ox) {
    int t = map[floor_idx][map_col].type;
    int won_combat = 0;

    if (t == NODE_MONSTER) {
        won_combat = run_combat(oy, ox, 0);
        if (!won_combat || over) return;
        gold += 5 + floor_idx;
        card_reward(oy, ox);
        hp += has_relic(RELIC_HEAL_AMULET) ? 5 : 0;
        if (hp > max_hp) hp = max_hp;
    } else if (t == NODE_ELITE) {
        won_combat = run_combat(oy, ox, 1);
        if (!won_combat || over) return;
        gold += 10 + floor_idx * 2;
        card_reward(oy, ox);
        relic_reward(oy, ox);
        hp += has_relic(RELIC_HEAL_AMULET) ? 5 : 0;
        if (hp > max_hp) hp = max_hp;
    } else if (t == NODE_BOSS) {
        won_combat = run_combat(oy, ox, 1);
        if (!won_combat || over) return;
        won = 1;
        return;
    } else if (t == NODE_REST) {
        do_rest(oy, ox);
    } else if (t == NODE_SHOP) {
        do_shop(oy, ox);
    } else if (t == NODE_TREASURE) {
        relic_reward(oy, ox);
    }
}

/* ───────── Main ───────── */
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
        int oy = (LINES - 24) / 2;
        int ox = (COLS - 60) / 2;

        while (!over && !won && floor_idx < MAP_ROWS) {
            if (map_screen(oy, ox) == 0) { over = 1; break; }
            process_node(oy, ox);
            if (over) break;
            if (won) break;
            floor_idx++;
        }

        erase();
        if (over) {
            attron(COLOR_PAIR(3) | A_BOLD);
            mvaddstr(oy + 8, ox + 15, _("GAME OVER", "FIM DE JOGO"));
            attroff(COLOR_PAIR(3) | A_BOLD);
        } else if (won) {
            attron(COLOR_PAIR(2) | A_BOLD);
            mvaddstr(oy + 6, ox + 10, _("THE SPIRE IS CONQUERED!",
                                        "A SPIRE FOI CONQUISTADA!"));
            attroff(COLOR_PAIR(2) | A_BOLD);
        }
        attron(COLOR_PAIR(5));
        char buf[64];
        snprintf(buf, 64, _("Reached floor: %d", "Andar alcancado: %d"),
                 floor_idx + 1);
        mvaddstr(oy + 10, ox + 12, buf);
        int cnt = 0;
        for (int i = 0; i < relic_sz; i++) {
            mvaddstr(oy + 12 + cnt, ox + 12, relic_names[relics[i]]);
            cnt++;
        }
        mvaddstr(oy + 12 + cnt + 1, ox + 8,
                 _("Enter: play again | Q: quit",
                   "Enter: jogar novamente | Q: sair"));
        attroff(COLOR_PAIR(5));
        refresh();
        nodelay(stdscr, FALSE);
        while (1) {
            int c = getch();
            if (c == 'q') { play_again = 0; break; }
            if (c == '\n') break;
        }
        nodelay(stdscr, TRUE);
    }

    endwin();
    return 0;
}
