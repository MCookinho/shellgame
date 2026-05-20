/******************************************************************************
 *                         SUPER AUTO PETS
 *     An auto-battler roguelike — collect pets, fight, and survive!
 ******************************************************************************/
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "config.h"

/* ───────── Constants ───────── */
#define MAX_TEAM 5
#define SHOP_PETS 4
#define SHOP_FOOD 2
#define MAX_SHOP (SHOP_PETS + SHOP_FOOD)
#define START_LIVES 10
#define START_GOLD 10

/* ───────── Ability enum ───────── */
typedef enum {
    ABL_NONE,
    ABL_FAINT_CRICKET,
    ABL_FAINT_SPIDER,
    ABL_FAINT_SHEEP,
    ABL_BUY_HORSE,
    ABL_BUYFOOD_RABBIT,
    ABL_HURT_PEACOCK,
    ABL_ALLYFAINT_OX,
    ABL_START_OTTER,
    ABL_START_MOSQ,
    ABL_START_SWAN,
    ABL_START_WHALE,
    ABL_SNAIL,
    ABL_ELEPHANT,
} Ability;

/* ───────── Pet template ───────── */
typedef struct {
    const char *name;
    int atk, hp, tier, cost;
    Ability abl;
} PetTmpl;

/* ───────── Pet instance ───────── */
typedef struct {
    int id;          /* index into PETS */
    int atk, hp, max_hp;
    int honey;       /* spawns 1/1 bee on faint */
} Pet;

/* ───────── Food template ───────── */
typedef struct {
    const char *name;
    int atk_b, hp_b, cost;
} FoodTmpl;

/* ───────── Team ───────── */
typedef struct {
    Pet slots[MAX_TEAM];
    int n;
} Team;

/* ───────── Game state ───────── */
typedef struct {
    Team team;
    int gold, lives, round, streak, lost_last;
    int shop_ids[MAX_SHOP];   /* indices into PETS (pets) or -1 for food */
    int shop_food[MAX_SHOP];  /* food indices */
    int shop_type[MAX_SHOP];  /* 0=pet, 1=food */
} State;

static State S;

/* ───────── Pet templates ───────── */
static PetTmpl PETS[] = {
    {"Ant",      2, 1, 1, 2, ABL_NONE},
    {"Cricket",  1, 2, 1, 2, ABL_FAINT_CRICKET},
    {"Fish",     2, 2, 1, 2, ABL_NONE},
    {"Horse",    1, 3, 1, 2, ABL_BUY_HORSE},
    {"Mosquito", 2, 2, 1, 2, ABL_START_MOSQ},
    {"Otter",    1, 2, 1, 2, ABL_START_OTTER},
    {"Peacock",  1, 2, 2, 3, ABL_HURT_PEACOCK},
    {"Rabbit",   2, 2, 2, 3, ABL_BUYFOOD_RABBIT},
    {"Sheep",    2, 2, 2, 3, ABL_FAINT_SHEEP},
    {"Spider",   2, 2, 2, 3, ABL_FAINT_SPIDER},
    {"Swan",     1, 2, 2, 3, ABL_START_SWAN},
    {"Snail",    2, 2, 2, 3, ABL_SNAIL},
    {"Ox",       1, 3, 3, 4, ABL_ALLYFAINT_OX},
    {"Whale",    3, 4, 3, 4, ABL_START_WHALE},
    {"Elephant", 3, 5, 3, 4, ABL_ELEPHANT},
};
#define PET_COUNT ((int)(sizeof(PETS) / sizeof(PETS[0])))

/* ───────── Food templates ───────── */
static FoodTmpl FOODS[] = {
    {"Apple",  1, 1, 2},
    {"Honey",  0, 0, 2},
    {"Meat",   2, 0, 2},
    {"Milk",   0, 2, 2},
    {"Pizza",  2, 2, 3},
};
#define FOOD_COUNT ((int)(sizeof(FOODS) / sizeof(FOODS[0])))

/* ───────── Helpers ───────── */
static int rng(int n) { return rand() % n; }

/* ───────── Pet instance constructor ───────── */
static Pet make_pet(int id) {
    Pet p;
    p.id = id;
    p.atk = PETS[id].atk;
    p.hp = PETS[id].hp;
    p.max_hp = PETS[id].hp;
    p.honey = 0;
    return p;
}

/* ───────── Shop generation ───────── */
static void gen_shop(void) {
    int max_tier = 1;
    if (S.round >= 5) max_tier = 3;
    else if (S.round >= 3) max_tier = 2;

    for (int i = 0; i < MAX_SHOP; i++) {
        if (i < SHOP_PETS) {
            int id;
            do {
                id = rng(PET_COUNT);
            } while (PETS[id].tier > max_tier);
            S.shop_ids[i] = id;
            S.shop_type[i] = 0;
        } else {
            S.shop_ids[i] = rng(FOOD_COUNT);
            S.shop_type[i] = 1;
        }
    }
}

/* ───────── Add pet to team (first empty slot) ───────── */
static int team_add(Pet p) {
    if (S.team.n >= MAX_TEAM) return 0;
    S.team.slots[S.team.n++] = p;
    return 1;
}

/* ───────── Remove pet at index, shift left ───────── */
static void team_remove(int idx) {
    for (int i = idx; i < S.team.n - 1; i++)
        S.team.slots[i] = S.team.slots[i + 1];
    S.team.n--;
}

/* ───────── Find existing pet of same type (for combining) ───────── */
static int team_find_same(int id) {
    for (int i = 0; i < S.team.n; i++)
        if (S.team.slots[i].id == id) return i;
    return -1;
}

/* ───────── Apply food to a pet ───────── */
static void apply_food(int pet_idx, int food_idx) {
    if (pet_idx < 0 || pet_idx >= S.team.n) return;
    FoodTmpl *f = &FOODS[food_idx];
    Pet *p = &S.team.slots[pet_idx];
    if (strcmp(f->name, "Honey") == 0) {
        p->honey = 1;
    } else {
        p->atk += f->atk_b;
        p->hp += f->hp_b;
        p->max_hp += f->hp_b;
    }
    /* Rabbit ability */
    for (int i = 0; i < S.team.n; i++) {
        if (PETS[S.team.slots[i].id].abl == ABL_BUYFOOD_RABBIT) {
            int target = rng(S.team.n);
            S.team.slots[target].hp++;
            S.team.slots[target].max_hp++;
            break;
        }
    }
}

/* ───────── Buy from shop ───────── */
static int shop_buy(int idx) {
    if (idx < 0 || idx >= MAX_SHOP) return 0;
    if (S.shop_ids[idx] < 0) return 0;

    if (S.shop_type[idx] == 0) { /* pet */
        int id = S.shop_ids[idx];
        int cost = PETS[id].cost;
        if (S.gold < cost) return 0;
        S.gold -= cost;

        /* Check for combine */
        int existing = team_find_same(id);
        if (existing >= 0) {
            /* Combine: +1 atk, +1 hp, +1 max_hp */
            S.team.slots[existing].atk++;
            S.team.slots[existing].hp++;
            S.team.slots[existing].max_hp++;
        } else {
            if (S.team.n >= MAX_TEAM) { S.gold += cost; return 0; }
            Pet p = make_pet(id);
            team_add(p);

            /* Horse ability */
            for (int i = 0; i < S.team.n; i++) {
                if (PETS[S.team.slots[i].id].abl == ABL_BUY_HORSE) {
                    S.team.slots[S.team.n - 1].atk++;
                    break;
                }
            }
        }
        S.shop_ids[idx] = -1;
        return 1;

    } else { /* food */
        int fid = S.shop_ids[idx];
        int cost = FOODS[fid].cost;
        if (S.gold < cost) return 0;
        if (S.team.n == 0) return 0;

        S.gold -= cost;
        /* Show pet selection prompt in main loop */
        /* Apply will be done after player selects target */
        return 2; /* signal: need pet selection */
    }
}

/* ───────── Sell pet ───────── */
static void sell_pet(int idx) {
    if (idx < 0 || idx >= S.team.n) return;
    S.gold += 1;
    team_remove(idx);
}

/* ───────── Generate enemy team ───────── */
static Team gen_enemy(void) {
    Team e;
    e.n = 0;
    int count = 1 + rng(3);
    if (count > MAX_TEAM) count = MAX_TEAM;
    for (int i = 0; i < count; i++) {
        int max_tier = 1;
        if (S.round >= 5) max_tier = 3;
        else if (S.round >= 3) max_tier = 2;
        int id;
        do { id = rng(PET_COUNT); } while (PETS[id].tier > max_tier);
        Pet p = make_pet(id);
        /* Random stat bonus based on round */
        int bonus = rng(S.round / 2 + 1);
        p.atk += bonus;
        p.hp += bonus;
        p.max_hp += bonus;
        e.slots[e.n++] = p;
    }
    return e;
}

/* ───────── Battle ───────── */
static int do_battle(int oy, int ox) {
    Team player, enemy;
    memcpy(&player, &S.team, sizeof(Team));
    Team e = gen_enemy();
    memcpy(&enemy, &e, sizeof(Team));

    /* ── Start-of-battle abilities ── */
    /* Whale: swallow front ally (protect) */
    for (int i = 0; i < player.n; i++) {
        if (PETS[player.slots[i].id].abl == ABL_START_WHALE && player.n > 1) {
            player.slots[i].hp += player.slots[0].hp;
            player.slots[i].atk++;
            /* Remove front ally (swallowed) */
            for (int k = 0; k < player.n - 1; k++)
                player.slots[k] = player.slots[k + 1];
            player.n--;
            i = -1;
        }
    }
    /* Elephant: deal 1 dmg to all enemies */
    for (int i = 0; i < player.n; i++) {
        if (PETS[player.slots[i].id].abl == ABL_ELEPHANT) {
            for (int j = 0; j < enemy.n; j++)
                enemy.slots[j].hp--;
        }
    }
    /* Otter: buff random friend */
    for (int i = 0; i < player.n; i++) {
        if (PETS[player.slots[i].id].abl == ABL_START_OTTER && player.n > 0) {
            int t = rng(player.n);
            player.slots[t].atk++;
            player.slots[t].hp++;
            player.slots[t].max_hp++;
        }
    }
    /* Mosquito: 1 dmg to random enemy */
    for (int i = 0; i < player.n; i++) {
        if (PETS[player.slots[i].id].abl == ABL_START_MOSQ) {
            int alive = 0;
            for (int j = 0; j < enemy.n; j++)
                if (enemy.slots[j].hp > 0) alive++;
            if (alive > 0) {
                int t;
                do { t = rng(enemy.n); } while (enemy.slots[t].hp <= 0);
                enemy.slots[t].hp--;
            }
        }
    }
    /* Swan: gain gold */
    for (int i = 0; i < player.n; i++) {
        if (PETS[player.slots[i].id].abl == ABL_START_SWAN)
            S.gold += 1;
    }
    /* Snail */
    for (int i = 0; i < player.n; i++) {
        if (PETS[player.slots[i].id].abl == ABL_SNAIL && S.lost_last) {
            player.slots[i].atk += 2;
            player.slots[i].hp += 2;
            player.slots[i].max_hp += 2;
        }
    }

    /* Remove dead enemies from start effects */
    for (int i = enemy.n - 1; i >= 0; i--)
        if (enemy.slots[i].hp <= 0)
            for (int j = i; j < enemy.n - 1; j++)
                enemy.slots[j] = enemy.slots[j + 1], enemy.n--;

    /* ── Combat loop ── */
    while (player.n > 0 && enemy.n > 0) {
        /* Front pets attack each other */
        Pet *a = &player.slots[0];
        Pet *b = &enemy.slots[0];

        /* a attacks b */
        b->hp -= a->atk;
        /* Peacock (enemy) */
        if (PETS[b->id].abl == ABL_HURT_PEACOCK && b->hp > 0)
            b->atk += 2;
        /* Ox (enemy) - ally faints later */
        /* Remove dead b */
        if (b->hp <= 0) {
            /* Honey */
            if (b->honey && enemy.n < MAX_TEAM) {
                Pet bee = make_pet(-1); /* custom: bee */
                bee.id = -1; bee.atk = 1; bee.hp = 1; bee.max_hp = 1;
                enemy.slots[enemy.n++] = bee;
            }
            /* Faint abilities */
            if (b->id >= 0 && PETS[b->id].abl == ABL_FAINT_CRICKET && enemy.n < MAX_TEAM) {
                Pet c = make_pet(-2);
                c.id = -2; c.atk = 1; c.hp = 1; c.max_hp = 1;
                enemy.slots[enemy.n++] = c;
            }
            if (b->id >= 0 && PETS[b->id].abl == ABL_FAINT_SPIDER && enemy.n < MAX_TEAM) {
                Pet c = make_pet(-3);
                c.id = -3; c.atk = 1; c.hp = 2; c.max_hp = 2;
                enemy.slots[enemy.n++] = c;
            }
            if (b->id >= 0 && PETS[b->id].abl == ABL_FAINT_SHEEP && enemy.n < MAX_TEAM) {
                Pet c = make_pet(-4);
                c.id = -4; c.atk = 2; c.hp = 2; c.max_hp = 2;
                enemy.slots[enemy.n++] = c;
            }
            /* Ox on ally faint */
            for (int k = 0; k < enemy.n; k++)
                if (PETS[enemy.slots[k].id].abl == ABL_ALLYFAINT_OX)
                    enemy.slots[k].atk += 2;
            /* Remove dead enemy */
            for (int k = 0; k < enemy.n - 1; k++)
                enemy.slots[k] = enemy.slots[k + 1];
            enemy.n--;
        }

        /* b attacks a (if still alive) */
        if (enemy.n > 0) {
            player.slots[0].hp -= enemy.slots[0].atk;
            if (PETS[player.slots[0].id].abl == ABL_HURT_PEACOCK && player.slots[0].hp > 0)
                player.slots[0].atk += 2;

            if (player.slots[0].hp <= 0) {
                if (player.slots[0].honey && player.n < MAX_TEAM) {
                    Pet bee = make_pet(-1);
                    bee.id = -1; bee.atk = 1; bee.hp = 1; bee.max_hp = 1;
                    player.slots[player.n++] = bee;
                }
                if (player.slots[0].id >= 0 && PETS[player.slots[0].id].abl == ABL_FAINT_CRICKET && player.n < MAX_TEAM) {
                    Pet c = make_pet(-2);
                    c.id = -2; c.atk = 1; c.hp = 1; c.max_hp = 1;
                    player.slots[player.n++] = c;
                }
                if (player.slots[0].id >= 0 && PETS[player.slots[0].id].abl == ABL_FAINT_SPIDER && player.n < MAX_TEAM) {
                    Pet c = make_pet(-3);
                    c.id = -3; c.atk = 1; c.hp = 2; c.max_hp = 2;
                    player.slots[player.n++] = c;
                }
                if (player.slots[0].id >= 0 && PETS[player.slots[0].id].abl == ABL_FAINT_SHEEP && player.n < MAX_TEAM) {
                    Pet c = make_pet(-4);
                    c.id = -4; c.atk = 2; c.hp = 2; c.max_hp = 2;
                    player.slots[player.n++] = c;
                }
                for (int k = 0; k < player.n; k++)
                    if (PETS[player.slots[k].id].abl == ABL_ALLYFAINT_OX)
                        player.slots[k].atk += 2;
                for (int k = 0; k < player.n - 1; k++)
                    player.slots[k] = player.slots[k + 1];
                player.n--;
            }
        }

        /* Display */
        erase();
        attron(COLOR_PAIR(2) | A_BOLD);
        char hdr[40];
        snprintf(hdr, 40, _("BATTLE — Round %d", "BATALHA — Rodada %d"), S.round);
        mvaddstr(oy, ox + 16, hdr);
        attroff(COLOR_PAIR(2) | A_BOLD);

        attron(COLOR_PAIR(5) | A_BOLD);
        mvaddstr(oy + 2, ox + 2, _("YOUR TEAM", "SEU TIME"));
        mvaddstr(oy + 2, ox + 38, _("ENEMY", "INIMIGO"));
        attroff(COLOR_PAIR(5) | A_BOLD);

        for (int i = 0; i < 5; i++) {
            int cy = oy + 4 + i;
            if (i < player.n) {
                Pet *p = &player.slots[i];
                const char *nm = (p->id >= 0) ? PETS[p->id].name :
                    (p->id == -1) ? "Bee" : (p->id == -2) ? "Cricket" :
                    (p->id == -3) ? "Spider" : "Ram";
                int filled = (p->hp > 0) ? (p->hp * 8 / (p->max_hp > 0 ? p->max_hp : 1)) : 0;
                if (filled > 8) filled = 8;
                if (filled < 0) filled = 0;
                attron(COLOR_PAIR(3) | A_BOLD);
                mvprintw(cy, ox + 2, "%c %s %d/%d ", 'A'+i, nm, p->atk, p->hp);
                attroff(COLOR_PAIR(3) | A_BOLD);
                attron(COLOR_PAIR(1));
                for (int b = 0; b < filled; b++) mvaddch(cy, ox + 18 + b, '#');
                attroff(COLOR_PAIR(1));
                attron(COLOR_PAIR(4) | A_DIM);
                for (int b = filled; b < 8; b++) mvaddch(cy, ox + 18 + b, '.');
                attroff(COLOR_PAIR(4) | A_DIM);
            } else {
                attron(COLOR_PAIR(4) | A_DIM);
                mvaddstr(cy, ox + 2, _("(empty)", "(vazio)"));
                attroff(COLOR_PAIR(4) | A_DIM);
            }
            if (i < enemy.n) {
                Pet *p = &enemy.slots[i];
                const char *nm = (p->id >= 0) ? PETS[p->id].name :
                    (p->id == -1) ? "Bee" : (p->id == -2) ? "Cricket" :
                    (p->id == -3) ? "Spider" : "Ram";
                int filled = (p->hp > 0) ? (p->hp * 8 / (p->max_hp > 0 ? p->max_hp : 1)) : 0;
                if (filled > 8) filled = 8;
                if (filled < 0) filled = 0;
                attron(COLOR_PAIR(3) | A_BOLD);
                mvprintw(cy, ox + 38, "%c %s %d/%d ", 'A'+i, nm, p->atk, p->hp);
                attroff(COLOR_PAIR(3) | A_BOLD);
                attron(COLOR_PAIR(1));
                for (int b = 0; b < filled; b++) mvaddch(cy, ox + 54 + b, '#');
                attroff(COLOR_PAIR(1));
                attron(COLOR_PAIR(4) | A_DIM);
                for (int b = filled; b < 8; b++) mvaddch(cy, ox + 54 + b, '.');
                attroff(COLOR_PAIR(4) | A_DIM);
            }
        }

        attron(COLOR_PAIR(5) | A_DIM);
        mvaddstr(oy + 11, ox + 2, _("Battle in progress...", "Batalha em andamento..."));
        attroff(COLOR_PAIR(5) | A_DIM);
        refresh();
        usleep(250000);
    }

    int won = (enemy.n == 0);
    S.lost_last = !won;
    erase();

    if (won) {
        S.streak++;
        int gold_earn = 2 + S.round / 2 + (S.streak >= 2 ? 1 : 0);
        S.gold += gold_earn;
        attron(COLOR_PAIR(2) | A_BOLD);
        mvaddstr(oy + 8, ox + 16, _("VICTORY!", "VITORIA!"));
        attroff(COLOR_PAIR(2) | A_BOLD);
        char buf[64];
        snprintf(buf, 64, _("+%d gold (streak: %d)", "+%d ouro (sequencia: %d)"), gold_earn, S.streak);
        attron(COLOR_PAIR(5));
        mvaddstr(oy + 10, ox + 16, buf);
        attroff(COLOR_PAIR(5));
    } else {
        S.streak = 0;
        S.lives--;
        attron(COLOR_PAIR(3) | A_BOLD);
        mvaddstr(oy + 8, ox + 16, _("DEFEAT!", "DERROTA!"));
        attroff(COLOR_PAIR(3) | A_BOLD);
        attron(COLOR_PAIR(5));
        char buf[64];
        snprintf(buf, 64, _("%d lives remaining", "%d vidas restantes"), S.lives);
        mvaddstr(oy + 10, ox + 14, buf);
        attroff(COLOR_PAIR(5));
    }

    attron(COLOR_PAIR(6) | A_DIM);
    mvaddstr(oy + 14, ox + 12,
             _("Press any key to continue...", "Pressione qualquer tecla..."));
    attroff(COLOR_PAIR(6) | A_DIM);
    refresh();
    nodelay(stdscr, FALSE);
    getch();
    nodelay(stdscr, TRUE);

    return won;
}

/* ───────── Display shop ───────── */
static void draw_shop(int oy, int ox) {
    erase();

    /* Header */
    attron(COLOR_PAIR(2) | A_BOLD);
    char hdr[48];
    snprintf(hdr, 48, _("SUPER AUTO PETS — Round %d", "SUPER AUTO PETS — Rodada %d"), S.round);
    mvaddstr(oy, ox + 6, hdr);
    attroff(COLOR_PAIR(2) | A_BOLD);

    char statbuf[64];
    snprintf(statbuf, 64, _("Gold: %d     Lives: %d     Streak: %d",
                             "Ouro: %d     Vidas: %d     Sequencia: %d"),
             S.gold, S.lives, S.streak);
    attron(COLOR_PAIR(5));
    mvaddstr(oy + 1, ox + 8, statbuf);
    attroff(COLOR_PAIR(5));

    /* Separator */
    attron(COLOR_PAIR(4) | A_DIM);
    for (int x = 0; x < 56; x++) mvaddch(oy + 2, ox + 2 + x, '-');
    attroff(COLOR_PAIR(4) | A_DIM);

    /* Team */
    attron(COLOR_PAIR(6) | A_BOLD);
    mvaddstr(oy + 3, ox + 2, _("TEAM:", "TIME:"));
    attroff(COLOR_PAIR(6) | A_BOLD);

    for (int i = 0; i < MAX_TEAM; i++) {
        int cy = oy + 4 + i;
        if (i < S.team.n) {
            Pet *p = &S.team.slots[i];
            const char *nm = (p->id >= 0) ? PETS[p->id].name : "???";
            attron(COLOR_PAIR(3) | A_BOLD);
            mvprintw(cy, ox + 4, "%d. %s [%d/%d]", i + 1, nm, p->atk, p->hp);
            attroff(COLOR_PAIR(3) | A_BOLD);
            /* HP bar */
            int filled = (p->hp > 0) ? (p->hp * 10 / (p->max_hp > 0 ? p->max_hp : 1)) : 0;
            if (filled > 10) filled = 10;
            if (filled < 0) filled = 0;
            mvaddch(cy, ox + 28, '[');
            attron(COLOR_PAIR(1));
            for (int b = 0; b < filled; b++) mvaddch(cy, ox + 29 + b, '#');
            attroff(COLOR_PAIR(1));
            attron(COLOR_PAIR(4) | A_DIM);
            for (int b = filled; b < 10; b++) mvaddch(cy, ox + 29 + b, '.');
            attroff(COLOR_PAIR(4) | A_DIM);
            mvaddch(cy, ox + 39, ']');
            if (p->honey) {
                attron(COLOR_PAIR(5));
                mvaddstr(cy, ox + 41, "🍯");
                attroff(COLOR_PAIR(5));
            }
        } else {
            attron(COLOR_PAIR(4) | A_DIM);
            mvaddstr(cy, ox + 4, _("(empty slot)", "(vazio)"));
            attroff(COLOR_PAIR(4) | A_DIM);
        }
    }

    /* Separator */
    attron(COLOR_PAIR(4) | A_DIM);
    for (int x = 0; x < 56; x++) mvaddch(oy + 10, ox + 2 + x, '-');
    attroff(COLOR_PAIR(4) | A_DIM);

    /* Shop */
    attron(COLOR_PAIR(6) | A_BOLD);
    mvaddstr(oy + 11, ox + 2, _("SHOP:", "LOJA:"));
    attroff(COLOR_PAIR(6) | A_BOLD);

    char labels[] = "abcdefgh";
    for (int i = 0; i < MAX_SHOP; i++) {
        int cy = oy + 12 + i;
        if (S.shop_ids[i] < 0) {
            attron(COLOR_PAIR(4) | A_DIM);
            mvaddstr(cy, ox + 4, _("(sold)", "(vendido)"));
            attroff(COLOR_PAIR(4) | A_DIM);
            continue;
        }
        if (S.shop_type[i] == 0) {
            int id = S.shop_ids[i];
            char tier_s[4];
            for (int t = 0; t < PETS[id].tier; t++) tier_s[t] = '*';
            tier_s[PETS[id].tier] = '\0';
            attron(COLOR_PAIR(3) | A_BOLD);
            mvprintw(cy, ox + 4, "%c) %s [%d/%d]  %dg  %s",
                     labels[i], PETS[id].name, PETS[id].atk, PETS[id].hp,
                     PETS[id].cost, tier_s);
            attroff(COLOR_PAIR(3) | A_BOLD);
        } else {
            int fid = S.shop_ids[i];
            FoodTmpl *f = &FOODS[fid];
            attron(COLOR_PAIR(5));
            mvprintw(cy, ox + 4, "%c) %s", labels[i], f->name);
            if (f->atk_b > 0 || f->hp_b > 0)
                mvprintw(cy, ox + 18, "+%d/+%d", f->atk_b, f->hp_b);
            else
                mvaddstr(cy, ox + 18, _("🐝 faint bee", "🐝 abelha"));
            mvprintw(cy, ox + 30, "%dg", f->cost);
            attroff(COLOR_PAIR(5));
        }
    }

    /* Separator */
    attron(COLOR_PAIR(4) | A_DIM);
    for (int x = 0; x < 56; x++) mvaddch(oy + 19, ox + 2 + x, '-');
    attroff(COLOR_PAIR(4) | A_DIM);

    /* Controls */
    attron(COLOR_PAIR(6) | A_DIM);
    mvaddstr(oy + 20, ox + 2,
             _("a-f: buy  | S s: sell pet N  | Enter: battle!  | Q: quit",
               "a-f: comprar  | S s: vender pet N  | Enter: batalha!  | Q: sair"));
    attroff(COLOR_PAIR(6) | A_DIM);
}

/* ───────── Shop input loop ───────── */
static int shop_loop(int oy, int ox) {
    draw_shop(oy, ox);
    refresh();

    int ch = getch();
    if (ch == 'q') return -1;

    /* Sell mode */
    if (ch == 'S' || ch == 's') {
        draw_shop(oy, ox);
        attron(COLOR_PAIR(3) | A_BOLD);
        mvaddstr(oy + 21, ox + 2,
                 _("Sell which pet? (1-5)", "Vender qual pet? (1-5)"));
        attroff(COLOR_PAIR(3) | A_BOLD);
        refresh();
        nodelay(stdscr, FALSE);
        int c = getch();
        nodelay(stdscr, TRUE);
        if (c >= '1' && c <= '5') {
            int idx = c - '1';
            if (idx < S.team.n) sell_pet(idx);
        }
        return 0;
    }

    /* Buy */
    if (ch >= 'a' && ch <= 'h') {
        int idx = ch - 'a';
        if (idx >= MAX_SHOP || S.shop_ids[idx] < 0) return 0;
        int result = shop_buy(idx);
        if (result == 2) {
            /* Need to select target pet for food */
            draw_shop(oy, ox);
            attron(COLOR_PAIR(3) | A_BOLD);
            mvaddstr(oy + 21, ox + 2,
                     _("Apply food to which pet? (1-5)",
                       "Aplicar comida em qual pet? (1-5)"));
            attroff(COLOR_PAIR(3) | A_BOLD);
            refresh();
            nodelay(stdscr, FALSE);
            int c = getch();
            nodelay(stdscr, TRUE);
            if (c >= '1' && c <= '5') {
                int pidx = c - '1';
                int fid = S.shop_ids[idx];
                apply_food(pidx, fid);
                S.shop_ids[idx] = -1;
            } else {
                /* Cancel - refund */
                S.gold += FOODS[S.shop_ids[idx]].cost;
            }
        }
        return 0;
    }

    /* Enter — battle */
    if (ch == '\n' || ch == ' ') {
        if (S.team.n == 0) return 0;
        do_battle(oy, ox);
        if (S.lives <= 0) return -1;
        S.round++;
        gen_shop();
        return 0;
    }

    return 0;
}

/* ───────── Game loop ───────── */
static void game_loop(void) {
    S.team.n = 0;
    S.gold = START_GOLD;
    S.lives = START_LIVES;
    S.round = 1;
    S.streak = 0;
    S.lost_last = 0;

    /* Starter pets */
    int starters[] = {0, 2, 4};
    for (int i = 0; i < 3; i++) {
        team_add(make_pet(starters[i]));
    }

    gen_shop();

    int oy = (LINES - 22) / 2;
    int ox = (COLS - 60) / 2;

    while (S.lives > 0) {
        int result = shop_loop(oy, ox);
        if (result < 0) break;
    }

    /* Game over screen */
    erase();
    int go_y = (LINES - 8) / 2;
    int go_x = (COLS - 40) / 2;
    if (S.lives <= 0) {
        attron(COLOR_PAIR(3) | A_BOLD);
        mvaddstr(go_y, go_x, _("GAME OVER", "FIM DE JOGO"));
        attroff(COLOR_PAIR(3) | A_BOLD);
    }
    char buf[64];
    snprintf(buf, 64, _("Reached round %d!", "Chegou na rodada %d!"), S.round);
    attron(COLOR_PAIR(5));
    mvaddstr(go_y + 2, go_x, buf);
    attroff(COLOR_PAIR(5));
    attron(COLOR_PAIR(6) | A_DIM);
    mvaddstr(go_y + 5, go_x - 6,
             _("Press Enter to return to menu...",
               "Pressione Enter para voltar ao menu..."));
    attroff(COLOR_PAIR(6) | A_DIM);
    refresh();
    nodelay(stdscr, FALSE);
    while (getch() != '\n');
    nodelay(stdscr, TRUE);
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
    init_theme_pair(6, COLOR_MAGENTA, COLOR_BLACK);
    apply_theme_bg();

    int keep_playing = 1;
    while (keep_playing) {
        game_loop();
        /* After game over loop — ask to play again? auto-quit */
        keep_playing = 0;
    }

    endwin();
    return 0;
}
