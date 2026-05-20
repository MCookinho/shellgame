/******************************************************************************
 *                            ASTEROIDS
 *        Navigate, shoot and destroy asteroids in deep space!
 ******************************************************************************/
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include "config.h"

#define MAX_BULLETS 15
#define MAX_ASTEROIDS 24
#define DIRS 8
#define THRUST 0.18f
#define FRICTION 0.988f
#define MAX_SPEED 5.5f
#define BULLET_SPEED 5.0f
#define BULLET_LIFE 28
#define BASE_ASTEROIDS 4
#define ASTEROID_SPEED 1.0f
#define SAFE_RADIUS 5

static const char dir_char[DIRS] = {'^','/','>','\\','v','/','<','\\'};
static const float dx[DIRS] = {0,0.707f,1,0.707f,0,-0.707f,-1,-0.707f};
static const float dy[DIRS] = {-1,-0.707f,0,0.707f,1,0.707f,0,-0.707f};

/* ───────── Entities ───────── */
typedef struct { float x,y,vx,vy; int life; } Bullet;
static Bullet bullets[MAX_BULLETS];
static int bcount;

typedef struct { float x,y,vx,vy; int size, active; } Asteroid;
static Asteroid roids[MAX_ASTEROIDS];
static int rcount;

static float sx,sy,svx,svy;
static int sangle, salive;
static int score, level, over;
static int game_rows, game_cols;

/* ───────── Utilities ───────── */
static int rng(int n) { return rand()%n; }
static void wrap(float *v, int m) { while(*v<0)*v+=m; while(*v>=m)*v-=m; }
static float frnd(void) { return (float)rng(1000)/1000.0f; }

/* ───────── Spawn asteroid ───────── */
static void add_roid(float x,float y,int sz,float vx,float vy) {
    if(rcount>=MAX_ASTEROIDS)return;
    Asteroid *a=&roids[rcount++];
    a->x=x; a->y=y; a->size=sz; a->active=1;
    a->vx=vx; a->vy=vy;
}

static void spawn_roids(int n,int sz) {
    for(int i=0;i<n;i++){
        float a=frnd()*6.2832f,sp=ASTEROID_SPEED*(0.5f+frnd());
        if(sz==1)sp*=1.4f;
        if(sz==0)sp*=1.8f;
        float x,y; int att=0;
        do{
            x=2+frnd()*(game_cols-4);
            y=2+frnd()*(game_rows-4);
            att++;
        }while(att<50&&fabsf(x-sx)<SAFE_RADIUS&&fabsf(y-sy)<SAFE_RADIUS);
        add_roid(x,y,sz,cosf(a)*sp,sinf(a)*sp);
    }
}

/* ───────── Init level ───────── */
static void init_level(void) {
    rcount=0; bcount=0;
    sx=game_cols/2.0f; sy=game_rows/2.0f;
    svx=0; svy=0; sangle=0;
    salive=1;
    int n=BASE_ASTEROIDS+level-1;
    if(n>MAX_ASTEROIDS)n=MAX_ASTEROIDS;
    spawn_roids(n,2);
}

static void init_game(void) {
    score=0; level=1; over=0;
    init_level();
}

/* ───────── Split asteroid ───────── */
static void split_roid(int i) {
    Asteroid *a=&roids[i];
    int nsz=a->size-1;
    if(nsz<0){a->active=0;return;}
    for(int j=0;j<2;j++){
        float ang=frnd()*6.2832f,sp=ASTEROID_SPEED*(0.8f+frnd()*0.6f);
        if(nsz==0)sp*=1.4f;
        add_roid(a->x,a->y,nsz,cosf(ang)*sp,sinf(ang)*sp);
    }
    a->active=0;
}

/* ───────── Shoot ───────── */
static void shoot(void) {
    if(bcount>=MAX_BULLETS)return;
    Bullet *b=&bullets[bcount++];
    b->x=sx+dx[sangle]*2; b->y=sy+dy[sangle]*2;
    b->vx=dx[sangle]*BULLET_SPEED; b->vy=dy[sangle]*BULLET_SPEED;
    b->life=BULLET_LIFE;
}

/* ───────── Update ───────── */
static void update(void) {
    if(salive){
        svx*=FRICTION; svy*=FRICTION;
        sx+=svx; sy+=svy;
        float sp=sqrtf(svx*svx+svy*svy);
        if(sp>MAX_SPEED){svx=svx/sp*MAX_SPEED;svy=svy/sp*MAX_SPEED;}
        wrap(&sx,game_cols); wrap(&sy,game_rows);
    }

    for(int i=0;i<bcount;i++){
        if(!bullets[i].life)continue;
        bullets[i].x+=bullets[i].vx;
        bullets[i].y+=bullets[i].vy;
        wrap(&bullets[i].x,game_cols); wrap(&bullets[i].y,game_rows);
        bullets[i].life--;
    }

    for(int i=0;i<rcount;i++){
        if(!roids[i].active)continue;
        roids[i].x+=roids[i].vx;
        roids[i].y+=roids[i].vy;
        wrap(&roids[i].x,game_cols); wrap(&roids[i].y,game_rows);
    }

    for(int i=0;i<bcount;i++){
        if(!bullets[i].life)continue;
        for(int j=0;j<rcount;j++){
            if(!roids[j].active)continue;
            float d=sqrtf(powf(bullets[i].x-roids[j].x,2)
                         +powf(bullets[i].y-roids[j].y,2));
            if(d<1.5f){
                bullets[i].life=0;
                int pts=(roids[j].size==2)?20:(roids[j].size==1)?50:100;
                score+=pts;
                split_roid(j);
                break;
            }
        }
    }

    if(salive){
        for(int j=0;j<rcount;j++){
            if(!roids[j].active)continue;
            float d=sqrtf(powf(sx-roids[j].x,2)+powf(sy-roids[j].y,2));
            if(d<1.5f){salive=0;over=1;return;}
        }
    }

    int w=0;
    for(int i=0;i<bcount;i++)if(bullets[i].life)bullets[w++]=bullets[i];
    bcount=w;

    w=0; int any=0;
    for(int i=0;i<rcount;i++)if(roids[i].active){roids[w++]=roids[i];any=1;}
    rcount=w;

    if(!any&&salive){level++;init_level();}
}

/* ───────── Draw ───────── */
static void draw(int oy,int ox) {
    erase();
    for(int i=0;i<rcount;i++){
        if(!roids[i].active)continue;
        int x=(int)roids[i].x,y=(int)roids[i].y;
        char ch; int col;
        switch(roids[i].size){
            case 2:ch='@';col=3;break;
            case 1:ch='O';col=5;break;
            case 0:ch='o';col=6;break;
            default:ch='?';col=4;break;
        }
        attron(COLOR_PAIR(col)|A_BOLD);
        mvaddch(oy+y,ox+x,ch);
        attroff(COLOR_PAIR(col)|A_BOLD);
    }

    for(int i=0;i<bcount;i++){
        if(!bullets[i].life)continue;
        int x=(int)bullets[i].x,y=(int)bullets[i].y;
        attron(COLOR_PAIR(2)|A_BOLD);
        mvaddch(oy+y,ox+x,'.');
        attroff(COLOR_PAIR(2)|A_BOLD);
    }

    if(salive){
        int x=(int)sx,y=(int)sy;
        attron(COLOR_PAIR(1)|A_BOLD);
        mvaddch(oy+y,ox+x,dir_char[sangle]);
        attroff(COLOR_PAIR(1)|A_BOLD);
    }

    attron(COLOR_PAIR(2)|A_BOLD);
    char hdr[24];
    snprintf(hdr,24,_("ASTEROIDS","ASTEROIDES"));
    mvaddstr(oy-1,ox+game_cols/2-5,hdr);
    attroff(COLOR_PAIR(2)|A_BOLD);

    char info[80];
    snprintf(info,80,_("Score: %d  Level: %d","Pontos: %d  Nivel: %d"),
             score,level);
    attron(COLOR_PAIR(5));
    mvaddstr(oy-1,ox+2,info);
    attroff(COLOR_PAIR(5));

    int remaining=0;
    for(int i=0;i<rcount;i++)if(roids[i].active)remaining++;
    snprintf(info,80,_("Asteroids: %d","Asteroides: %d"),remaining);
    attron(COLOR_PAIR(6));
    mvaddstr(oy-1,ox+game_cols-20,info);
    attroff(COLOR_PAIR(6));

    if(!salive){
        attron(COLOR_PAIR(3)|A_BOLD);
        mvaddstr(oy+game_rows/2,ox+game_cols/2-5,
                 _("GAME OVER","FIM DE JOGO"));
        attroff(COLOR_PAIR(3)|A_BOLD);
        attron(COLOR_PAIR(6)|A_DIM);
        mvaddstr(oy+game_rows/2+2,ox+game_cols/2-12,
                 _("Enter: play again  Q: quit",
                   "Enter: jogar novamente  Q: sair"));
        attroff(COLOR_PAIR(6)|A_DIM);
    }

    attron(COLOR_PAIR(4)|A_DIM);
    mvaddstr(oy+game_rows+1,ox+2,
             _("<- ->: turn  ^: thrust  Space: shoot  Q: quit",
               "<- ->: girar  ^: impulso  Espaco: tiro  Q: sair"));
    attroff(COLOR_PAIR(4)|A_DIM);
    refresh();
}

int main(void) {
    srand(time(NULL));
    read_config();
    initscr(); cbreak(); noecho(); curs_set(0);
    keypad(stdscr,TRUE); nodelay(stdscr,TRUE);
    start_color();

    init_theme_pair(1,COLOR_GREEN,COLOR_BLACK);
    init_theme_pair(2,COLOR_YELLOW,COLOR_BLACK);
    init_theme_pair(3,COLOR_RED,COLOR_BLACK);
    init_theme_pair(4,COLOR_WHITE,COLOR_BLACK);
    init_theme_pair(5,COLOR_CYAN,COLOR_BLACK);
    init_theme_pair(6,COLOR_MAGENTA,COLOR_BLACK);
    apply_theme_bg();

    int term_rows,term_cols;
    getmaxyx(stdscr,term_rows,term_cols);
    game_cols=term_cols-2; game_rows=term_rows-4;
    int oy=2,ox=1;

    int play=1;
    while(play){
        init_game();
        while(1){
            int ch=getch();
            if(ch=='q'){play=0;break;}
            if(!over){
                if(ch==KEY_LEFT||ch=='a')sangle=(sangle-1+DIRS)%DIRS;
                if(ch==KEY_RIGHT||ch=='d')sangle=(sangle+1)%DIRS;
                if(ch==KEY_UP||ch=='w'){
                    svx+=dx[sangle]*THRUST;svy+=dy[sangle]*THRUST;
                }
                if(ch==' ')shoot();
                update();
            }else{
                if(ch=='\n'||ch==' '||ch=='y'){
                    if(ch!='q'){init_game();continue;}
                }
            }
            draw(oy,ox);
            usleep(30000);
        }
        if(!play)break;
    }

    endwin();
    return 0;
}
