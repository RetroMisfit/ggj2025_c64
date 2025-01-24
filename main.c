#include <string.h>

#define POKE(_a,_d) *(byte*)(_a) = (_d)
#define POKEW(_a,_d) *(word*)(_a) = (_d)
#define PEEK(_a) *(byte*)(_a)
#define PEEKW(_a) *(word*)(_a)
#define UP      0x01
#define DOWN    0x02
#define LEFT    0x04
#define RIGHT   0x08
#define FIRE    0x10

typedef unsigned char byte;
typedef unsigned short word;

#pragma rodata-name("CHAR")
#include "gfx/gfx.h"
#include "gfx/sprites.h"
#include "gfx/sprites_bw.h"
#pragma rodata-name("RODATA")

#pragma bss-name("BSS")
void __fastcall__ init_audio(void);
void __fastcall__ init_nmi(void);
void __fastcall__ start_music(void);
void __fastcall__ wait_raster(void);

#define CHAR_ADDR       0xC000
#define SCREEN_ADDR     0xC800
#define COLOR_ADDR      0xD800
#define SCREEN_TO_COL   (COLOR_ADDR-SCREEN_ADDR)

#define COLOR_ADDR      0xD800
#define SCREEN_W 40
#define SCREEN_H 25

#define PARAM1 0x33C

#define PS 1
#define BULLET_SPEED 8
#define VALUE_FREE 0x1FFF
#define MAX_ENEMIES 5
typedef struct 
{
    word x;
    word y;
    signed char sx;
    signed char sy;
} Enemy;

Enemy enemies[MAX_ENEMIES];

void hw_wait(byte _line) {c_a:POKE(PARAM1,_line);wait_raster();if (PEEK(0xD011)&0x80) goto c_a;}

byte i;
byte j;
byte k;
byte _i;
byte _j;
byte _joy;
byte joy;
byte* ptr;
byte* _ptr;
word px;
word py;
word bx;
word by; 
word wi;
byte bdir;
byte bnextdir;
signed char sx;
signed char sy;
signed char si;
byte counter;
byte bg_x;
byte bg_y;
byte state;
byte score;
byte show_score;
byte hiscore;
byte show_hiscore;

#define MENU 0
#define RUN  1
#define GAME_OVER 2 
#define START 3

#define SPRITE_X_OFFSET (20)
#define SPRITE_Y_OFFSET (44+7)
#define SPRITE_POINTER_ADDR (byte*)(SCREEN_ADDR+1016)
#define MAX_SPEED 4
#define MAX_ENEMY_SPEED 2
#define DEBUG(_d) POKE(0xD020,_d)
#define GETID(_x,_y) (_x+(_y)*bmp_items_w)
#define EMPTY_TILE GETID(5,3)
#define EMPTY_COL   6

void print(byte x, byte y, byte col, const char* txt)
{
    ptr = SCREEN_ADDR+x+y*SCREEN_W;
    next:
    _i = *txt;
    if (_i >= 'A' && _i <= ']') _i = _i-'A';
    else if (_i >= '0' && _i <= '9') _i = _i-'0'+28;
    else if (_i >= ':') {_i = GETID(6,3);}
    else if (_i >= ' ') {goto skip_space;}
    ptr[SCREEN_TO_COL] = col;
    *ptr = _i;
    skip_space:
    ++txt;
    ++ptr;
    if (*txt) goto next;
}

byte tmp[4];
void print_number(byte x, byte y, byte num)
{
    tmp[0] = '0';
    tmp[1] = '0';
    tmp[2] = '0';
    tmp[3] = 0;
    while (num > 100) {++tmp[0]; num -= 100;}
    while (num > 10) {++tmp[1]; num -= 10;}
    tmp[2] += num;
    print(x,y,8,tmp);
}

#define SPRITE_ENABLE(_id) POKE(0xD015,PEEK(0xD015)|(0x1<<(_id)))
#define SPRITE_DISABLE(_id) POKE(0xD015,PEEK(0xD015)&~(0x1<<(_id)))

void SPRITEX(byte id, word x) 
{
//    x += SPRITE_X_OFFSET;
    POKE(0xD000+(id)*2,x&0xFF);
    _j = 0x1<<id;
    if (x>0xff) 
    {
        _i = PEEK(0xD010)|_j;
    }
    else 
    {
        _i = PEEK(0xD010)&(~_j);
    }
    POKE(0xD010,_i);
}

#define SPRITEY(_id,_y) POKE(0xD001+(_id)*2,/*SPRITE_Y_OFFSET+*/(_y))
#define SPRITE_POINTER(_id,_p) POKE(SPRITE_POINTER_ADDR+(_id),15+_p)
#define SPRITE_COLOR(_id,_c) POKE(0xD027+(_id),_c)

void hw_read_input()
{
    joy = 0;
    _joy = PEEK(0xDC00);
    if (!(_joy&0x02)) joy |= DOWN;
    if (!(_joy&0x01)) joy |= UP;
    if (!(_joy&0x04)) joy |= LEFT;
    if (!(_joy&0x08)) joy |= RIGHT;
    if (!(_joy&0x10)) joy |= FIRE;
    _joy = PEEK(0xDC01);
    if (!(_joy&0x02)) joy |= DOWN;
    if (!(_joy&0x01)) joy |= UP;
    if (!(_joy&0x04)) joy |= LEFT;
    if (!(_joy&0x08)) joy |= RIGHT;
    if (!(_joy&0x10)) joy |= FIRE;
}

void hw_set_screen_state( byte state )
{    
    _i = PEEK(0xD011);
    if (!state)
    {
        _i &= 0xEF;
    }
    else 
    {
        _i |= 0x10;
    }
    POKE(0xD011,_i);
}

void draw(byte id, byte col,byte x, byte y, byte w, byte h)
{
    ptr = SCREEN_ADDR+x+y*SCREEN_W;
    next_y:
        memset(ptr,id,w);
        memset(&ptr[SCREEN_TO_COL],col,w);
        --h;
        if (h) 
        {
            ptr += SCREEN_W;
            goto next_y;
        }
}

const byte blink[8] = {1,15,12,11,11,12,15,1};
const byte fade[4] = {EMPTY_COL,2,7,1};
#define set_state(_s)  {state = _s; counter = 0;}

void main() 
{
    POKE(0x288,0x04);
    POKE(0xDD00,(PEEK(0xDD00)&0xFC)|0x00); // VIC bank
    POKE(0x01,0x35);
    memset(0x200,0,0xfd);
    POKE(0xD020,15); // border col
    // 00 bg col
    // 01 screen ram 0xf0
    // 10 screen ram 0x0f
    // 11 color ram
    hw_set_screen_state(0);
    POKE(0xD018,0x20); // character memory
    POKE(0xD021,0x00); // bg col // mc 00
    POKE(0xD022,0x0B); // ebg col 1 
    POKE(0xD023,0x01); // ebg col 2 
    POKE(0xD024,0x02); // ebg col 3 
    POKE(0xD015,0x00); // sprite enabled
    POKE(0xD017,0x00); // sprite double x
    POKE(0xD01D,0x00); // sprite double y
    POKE(0xD01C,0x01); // sprite multicolor
    //POKE(0xD01B,0xFC); // sprite priority
    POKE(0xD01B,0); // sprite priority

    POKE(0xD025,15); // sprite extra color 1
    POKE(0xD026,12); // sprite extra color 2

    px = ((SCREEN_W*8-12+SPRITE_X_OFFSET)/2)<<PS;
    py = (4*8/2+SPRITE_Y_OFFSET)<<PS;
    SPRITE_ENABLE(0);
    SPRITE_POINTER(0,0);
    SPRITE_COLOR(0,1);

    /*SPRITE_ENABLE(1);
    SPRITE_POINTER(1,4);
    SPRITE_COLOR(1,7);
    SPRITEX(1,50);
    SPRITEY(1,50);*/

    init_nmi();
    init_audio();
    start_music();
    score = 0; show_score = 50;
    hiscore = 0; show_hiscore = 50;

    POKE(0xD021,0);
    memset(SCREEN_ADDR,EMPTY_TILE,SCREEN_W*SCREEN_H);
    memset(COLOR_ADDR,EMPTY_COL,SCREEN_W*SCREEN_H);
    print(6,SCREEN_H/2-1,1,"MIKA MISFIT KER[NEN PRESENTS");
    print(6,SCREEN_H/2+1,2," GLOBAL GAME JAM 2025 GAME");
    print(6,SCREEN_H/2+3,7,"     LE BUBBL\\ MASSACR\\ ");
    print(6,SCREEN_H/2+5,2,"SHOOT ALL THOSE EVIL BUBBLES");

    draw(GETID(2,3),7, 0,0,1,1);
    draw(GETID(7,3),7, SCREEN_W-1,0,1,1);
    draw(GETID(8,3),7, 0,SCREEN_H-1,1,1);
    draw(GETID(9,3),7, SCREEN_W-1,SCREEN_H-1,1,1);

    draw(GETID(3,3),7, 1,0,SCREEN_W-2,1);
    draw(GETID(3,3),7, 1,SCREEN_H-1,SCREEN_W-2,1);
    draw(GETID(4,3),7, 0,1,1,SCREEN_H-2);
    draw(GETID(4,3),7, SCREEN_W-1,1,1,SCREEN_H-2);

    set_state(MENU);
    print(6,SCREEN_H-3,7,"     PRESS FIRE BUTTON ");
    print(3,SCREEN_H-1,1,"SCORE:000");
    print(SCREEN_W-9,SCREEN_H-1,1,"000:HI");

    sx = -2;
    sy = 0;
    SPRITE_ENABLE(1);
    SPRITE_POINTER(1,8);
    SPRITE_COLOR(1,15);

    bdir = UP;
    bnextdir = UP;
    bx = VALUE_FREE;

    i = 0;
    clear_next_enemy:
        enemies[i].x = VALUE_FREE;
        enemies[i].sx = 0;
        enemies[i].sy = 0;
    ++i; if (i != MAX_ENEMIES) goto clear_next_enemy;

    enemies[0].x = SPRITE_X_OFFSET;
    enemies[0].y = SPRITE_Y_OFFSET;
    SPRITE_ENABLE(2);
    SPRITE_POINTER(2,4);
    SPRITE_COLOR(2,7);

    hw_set_screen_state(1);
    loop:
    
    DEBUG(0);
    hw_wait(250);
    DEBUG(1);
    SPRITEX(0,px>>PS);
    SPRITEY(0,py>>PS);
    hw_read_input();
    if ((counter&0x03)==0)
    {
        if (joy&LEFT) 
        {
            if (sx > -MAX_SPEED) --sx;
            bnextdir = LEFT;
        }
        else if (joy&RIGHT) 
        {
            if (sx < MAX_SPEED) ++sx;
            bnextdir = RIGHT;
        }
        if (joy&UP) 
        {
            if (sy > -MAX_SPEED) --sy;
            bnextdir = UP;
        }
        else if (joy&DOWN) 
        {
            if (sy < MAX_SPEED) ++sy;
            bnextdir = DOWN;
        }
    }
    //i = 1; 
    if (joy&FIRE) 
    {
        //i = 2;
        if (state == RUN && bx == VALUE_FREE)
        {
            bx = px>>PS;
            by = py>>PS;
            bdir = bnextdir;
            SPRITE_ENABLE(1);
        }
    }
    px += sx;
    if (px < ((8+SPRITE_X_OFFSET)<<PS)) 
    {
        sx = -sx;
    }
    else if (px > (((SCREEN_W-1)*8-21+SPRITE_X_OFFSET)<<PS)) 
    {
        sx = -sx;
    }
    py += sy;
    if (py < ((8+SPRITE_Y_OFFSET)<<PS)) 
    {
        sy = -sy;
    }
    else if (py > (((SCREEN_H-1)*8-21+SPRITE_Y_OFFSET)<<PS))
    {
        sy = -sy;
    }
    ++counter;
    j = (counter>>2)&0x3;
    SPRITE_POINTER(0,j);

    i = bg_y>>2;
    POKE(CHAR_ADDR+GETID(5,3)*8+i,0x0);
    if (state == MENU) 
    {
        //++bg_x;
        ++bg_y;
    }
    bg_y+=sy;bg_y &= 0x1F;
    i = bg_y>>2;

    bg_x+=sx;bg_x &= 0x1F;
    j = 0x80>>(bg_x>>2);
    POKE(CHAR_ADDR+GETID(5,3)*8+i,j);

    if (state == START)
    {
        if ((counter&0x7)==0)
        {
            if (sy<0) ++sy;
            else if (sy>0) --sy;
            if (sx<0) ++sx;
            else if (sx>0) --sx;
        }
        i = counter>>1;
        if (i >= SCREEN_H+2) 
        {
            if (sx == 0 && sy == 0)
            {
                set_state(RUN);
            }
        }
        else 
        {
            j = 0;
            i = counter;
            next_line:
            k = 1-3+i+j;
            if (k >= 1 && k <= SCREEN_H-2)
            {
                memset(COLOR_ADDR+1+(k)*SCREEN_W,fade[j],SCREEN_W-2);
            }

            k = 0;
            wi = (SPRITE_Y_OFFSET+(1+i)*8)<<PS;
            next_enemy_pos:
            if (enemies[k].y < wi)
            {
                enemies[k].y = wi;
            }
            if (wi > (SCREEN_H*8+SPRITE_Y_OFFSET)<<PS)
            {
                enemies[k].x = VALUE_FREE;
            }
            ++k; if (k != MAX_ENEMIES) goto next_enemy_pos;
            if (i >= 0 && i <= SCREEN_H-3)
            {
                memset(SCREEN_ADDR+1+(1+i)*SCREEN_W,EMPTY_TILE,SCREEN_W-2);
            }
        }
    }
    else if (state == MENU)
    {
        i = (counter>>3)&0x7;
        memset(COLOR_ADDR+11+(SCREEN_H-3)*SCREEN_W,blink[i],5);
        memset(COLOR_ADDR+11+6+5+(SCREEN_H-3)*SCREEN_W,blink[i],6);
        i = (counter>>2)&0x7;
        memset(COLOR_ADDR+11+6+(SCREEN_H-3)*SCREEN_W,blink[i],4);
        if (joy&FIRE)
        {
            set_state(START);
        }
    }
    if (bx != VALUE_FREE)
    {
        SPRITEX(1,bx);
        SPRITEY(1,by);
        if (bx > (SCREEN_W-1)*8+SPRITE_X_OFFSET || by > (SCREEN_H-1)*8+SPRITE_Y_OFFSET)
        {
            SPRITE_DISABLE(1);
            bx = VALUE_FREE;
        } 
        else 
        {
            if (bdir == LEFT) bx-=BULLET_SPEED;
            else if (bdir == RIGHT) bx+=BULLET_SPEED;
            else if (bdir == UP) by-=BULLET_SPEED;
            else by+=BULLET_SPEED;
        }
    }
    i = 0;
    next_enemy:
    if (enemies[i].x != VALUE_FREE)
    {
        SPRITEX(2+i,enemies[i].x>>PS);
        SPRITEY(2+i,enemies[i].y>>PS);
        j = (counter>>3)&0x3;
        SPRITE_POINTER(2+i,4+j);
        if ((counter&0x3) == 0)
        {
            if (px < enemies[i].x) 
            {
                if (enemies[i].sx > -MAX_ENEMY_SPEED) --enemies[i].sx;
            }
            if (px > enemies[i].x) 
            {
                if (enemies[i].sx < MAX_ENEMY_SPEED) ++enemies[i].sx;
            }
            if (py < enemies[i].y) 
            {
                if (enemies[i].sy > -MAX_ENEMY_SPEED) --enemies[i].sy;
            }
            if (py > enemies[i].y) 
            {
                if (enemies[i].sy < MAX_ENEMY_SPEED) ++enemies[i].sy;
            }
        }
        enemies[i].x += enemies[i].sx;
        enemies[i].y += enemies[i].sy;
    }
    ++i; if (i != MAX_ENEMIES) goto next_enemy;

    if (show_score != score)
    {
        if (show_score < score) ++show_score; else --show_score;
        print_number(9,SCREEN_H-1,show_score);
    }
    if (show_hiscore != hiscore)
    {
        if (show_hiscore < hiscore) ++show_hiscore; else --show_hiscore;
        print_number(SCREEN_W-9,SCREEN_H-1,show_hiscore);
    }
    goto loop;
}