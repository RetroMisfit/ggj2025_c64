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
#define MAX_POWER 5
#define MAX_ENEMIES 5
#define TYPE_UP 0
#define TYPE_FOLLOW 1
#define TYPE_DOWN 2
#define TYPE_LEFT 3
#define TYPE_RIGHT 4

typedef struct 
{
    word x;
    word y;
    signed char sx;
    signed char sy;
    byte die;
    byte type;
} Enemy;

Enemy enemies[MAX_ENEMIES];
byte enemy_counter;

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
const byte* cptr;
word px;
word py;
word bx;
word by; 
word wi;
byte bdir;
byte bnextdir;
byte spawn;
byte speed;
byte pattern_counter;
byte level_counter;
const byte const pattern_easy[] = {SCREEN_W/2,SCREEN_H,TYPE_UP,10, SCREEN_W/2,0,TYPE_DOWN,10,0};
const byte const pattern_up[] = {SCREEN_W/2,SCREEN_H,TYPE_UP,2, 3,SCREEN_H,TYPE_UP,2, SCREEN_W-3,SCREEN_H,TYPE_UP,2, SCREEN_W/2,SCREEN_H,TYPE_UP,20,0};
const byte const pattern_down[] = {SCREEN_W/2,0,TYPE_DOWN,2, 3,0,TYPE_DOWN,2, SCREEN_W-3,0,TYPE_DOWN,2, SCREEN_W/2,0,TYPE_DOWN,2,0};
const byte const pattern_right[] = {1,SCREEN_H/2,TYPE_RIGHT,2, 1,3,TYPE_RIGHT,2, 1,SCREEN_H-3,TYPE_RIGHT,2, 1,SCREEN_H/2,TYPE_RIGHT,2,0};
const byte const pattern_left[] = {SCREEN_W,SCREEN_H/2,TYPE_LEFT,2, SCREEN_W,3,TYPE_LEFT,2, SCREEN_W,SCREEN_H-3,TYPE_LEFT,2, SCREEN_W,SCREEN_H/2,TYPE_LEFT,2,0};
const byte const pattern_corners[] ={1,0,TYPE_DOWN,3, SCREEN_W,SCREEN_H,TYPE_UP,3, SCREEN_W,0,TYPE_DOWN,3, 1,SCREEN_H,TYPE_UP,3, 0};

const byte const pattern_all[] = 
{
    SCREEN_W/2,SCREEN_H,TYPE_UP,2, 
    1,SCREEN_H/2,TYPE_RIGHT,2, 
    SCREEN_W/2,0,TYPE_DOWN,2, 
    SCREEN_W,SCREEN_H/2,TYPE_LEFT,20,
    0, 
};
const byte const pattern_all_follow[] = 
{
    SCREEN_W/2,SCREEN_H,TYPE_FOLLOW,1, 
    1,SCREEN_H/2,TYPE_FOLLOW,1, 
    SCREEN_W/2,0,TYPE_FOLLOW,1, 
    SCREEN_W,SCREEN_H/2,TYPE_FOLLOW,20,
    0, 
};

const byte* const level[] = 
{
    pattern_easy,
    pattern_left,
    pattern_right,
    pattern_down,
    pattern_up,
    pattern_corners,
    pattern_all,
    pattern_all_follow,
    0,
};

signed char sx;
signed char sy;
signed char si;
signed char sj;
signed short swi;
byte counter;
byte bg_x;
byte bg_y;
byte state;
word score;
word show_score;
word hiscore;
word show_hiscore;
byte tmp[6];
byte power;
word xx;
byte yy;
word xxp;
byte yyp;
byte blink_counter;

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
#define SPRITE_ENABLE(_id) POKE(0xD015,PEEK(0xD015)|(0x1<<(_id)))
#define SPRITE_DISABLE(_id) POKE(0xD015,PEEK(0xD015)&~(0x1<<(_id)))
#define SPRITEY(_id,_y) POKE(0xD001+(_id)*2,SPRITE_Y_OFFSET+(_y))
#define SPRITE_POINTER(_id,_p) POKE(SPRITE_POINTER_ADDR+(_id),15+_p)
#define SPRITE_COLOR(_id,_c) POKE(0xD027+(_id),_c)

void print(byte x, byte y, byte col, const char* txt)
{
    ptr = SCREEN_ADDR+x+y*SCREEN_W;
    next:
    _i = *txt;
    if (_i >= 'A' && _i <= ']') _i = _i-'A';
    else if (_i >= '0' && _i <= '9') _i = _i-'0'+28;
    else if (_i >= ':') {_i = GETID(6,3);}
    else if (_i >= '+') {_i = GETID(3,3);}
    else if (_i >= ' ') {goto skip_space;}
    ptr[SCREEN_TO_COL] = col;
    *ptr = _i;
    skip_space:
    ++txt;
    ++ptr;
    if (*txt) goto next;
}

void print_number(byte x, byte y, byte num)
{
    tmp[0] = '0';
    tmp[1] = '0';
    tmp[2] = '0';
    tmp[3] = 0;
    while (num >= 100) {++tmp[0]; num -= 100;}
    while (num >= 10) {++tmp[1]; num -= 10;}
    tmp[2] += num;
    print(x,y,8,tmp);
}

void print_number_word(byte x, byte y, word num)
{
    tmp[0] = '0';
    tmp[1] = '0';
    tmp[2] = '0';
    tmp[3] = '0';
    tmp[4] = '0';
    tmp[5] = 0;
    while (num >= 10000) {++tmp[0]; num -= 10000;}
    while (num >= 1000) {++tmp[1]; num -= 1000;}
    while (num >= 100) {++tmp[2]; num -= 100;}
    while (num >= 10) {++tmp[3]; num -= 10;}
    tmp[4] += num;
    print(x,y,8,tmp);
}

void SPRITEX(byte id, word x) 
{
    x += SPRITE_X_OFFSET;
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
const byte blink2[] = {0,11,11,12,12,15,15,1,1};

const char* const txt_menu       = "+++MENU+++";
const char* const txt_get_ready  = "+GET+READY";
const char* const txt_run        = "++MAYHEM++";
const char* txt_ptr; 
byte txt_cursor;

void set_state(byte s)  
{
    state = s; counter = 0;
    if (s == MENU)
    {
        txt_ptr = txt_menu;
    }
    else if (s == START)
    {
        txt_ptr = txt_get_ready;
    }
    else if (s == GAME_OVER)
    {
        txt_ptr = 0;
    }
    else
    {
        txt_ptr = txt_run;
    }
    txt_cursor = 0;
}

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
    POKE(0xD01B,0); // sprite priority

    POKE(0xD025,15); // sprite extra color 1
    POKE(0xD026,0); // sprite extra color 2

    SPRITE_ENABLE(0);
    SPRITE_POINTER(0,0);
    SPRITE_COLOR(0,1);

    init_nmi();
    init_audio();
    start_music();
    hiscore = 0; 
    POKE(0xD021,0);

    start_again:
    score = 0; 
    speed = 25;
    show_score = score+1;
    show_hiscore = hiscore+1;
    hw_set_screen_state(0);
    px = ((SCREEN_W*8-12)/2)<<PS;
    py = (4*8/2)<<PS;
    bx = VALUE_FREE;
    spawn = 0;
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

    print(16,SCREEN_H-1,1,"POWER:");
    memset(SCREEN_ADDR+22+(SCREEN_H-1)*SCREEN_W,GETID(0,5),MAX_POWER);
    memset(COLOR_ADDR+22+(SCREEN_H-1)*SCREEN_W,8,MAX_POWER);

    sx = -2;
    sy = 0;
    SPRITE_ENABLE(1);
    SPRITEX(1,-SPRITE_X_OFFSET);
    SPRITE_POINTER(1,9);
    SPRITE_COLOR(1,15);

    bdir = UP;
    bnextdir = UP;
    bx = VALUE_FREE;

    i = 0;
    clear_next_enemy:
        enemies[i].x = VALUE_FREE;
        enemies[i].sx = 0;
        enemies[i].sy = 0;
        SPRITE_ENABLE(2+i);
        SPRITE_POINTER(2+i,4);
        SPRITE_COLOR(2+i,7);
        SPRITEX(2+i,-SPRITE_X_OFFSET);
    ++i; if (i != MAX_ENEMIES) goto clear_next_enemy;

    power = MAX_POWER;
    enemy_counter = 0;
    pattern_counter = 0;
    level_counter = 0;
    blink_counter = 0;

    hw_set_screen_state(1);
    loop:
    
    if (state == START || state == GAME_OVER)
    {
        hw_wait(0);
        if (state == GAME_OVER)
        {
            i = ((counter>>2)&0x1)<<1;
            POKE(0xD016,0xC8|i);
            DEBUG(0);
        }
        else 
        {
            POKE(0xD016,0xC8);
            i = (counter>>1)&0x7;
            DEBUG(blink[i]);
        }
    }
    else 
    {
        if (blink_counter)
        {
            --blink_counter;
            hw_wait(0);
            DEBUG(blink2[blink_counter]);
        }
        else 
        {
            POKE(0xD016,0xC8);
            hw_wait(250);
            DEBUG(0);
            POKE(0xD021,0);
        }
    }
    SPRITEX(0,px>>PS);
    SPRITEY(0,py>>PS);
    hw_read_input();
    if (state == GAME_OVER)
    {
        if (sx > 0) --sx;
        else if (sx < 0) ++sx;
        if (sy > 0) --sy;
        else if (sy < 0) ++sy;
        if (counter == 200)
        {
            if (hiscore < score)
            {
                hiscore = score;
            }
            goto start_again;
        }
    }
    else if ((counter&0x03)==0)
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
    if (px < (8<<PS)) 
    {
        sx = -sx;
        px = 8<<PS;
    }
    else if (px > (((SCREEN_W-1)*8-21)<<PS)) 
    {
        sx = -sx;
        px = ((SCREEN_W-1)*8-21)<<PS;
    }
    py += sy;
    if (py < (8<<PS)) 
    {
        sy = -sy;
        py = 8<<PS;
    }
    else if (py > ((SCREEN_H-1)*8-21)<<PS)
    {
        sy = -sy;
        py = ((SCREEN_H-1)*8-21)<<PS;
    }
    ++counter;
    j = (counter>>2)&0x3;
    if (state == GAME_OVER)
    {
        SPRITE_POINTER(0,2+(j&0x1)*2);
    }
    else 
    {
        SPRITE_POINTER(0,j);
    }
    /*i = bg_y>>2;
    POKE(CHAR_ADDR+GETID(5,3)*8+i,0x0);
    if (state == MENU) 
    {
        ++bg_y;
    }
    bg_y+=sy;bg_y &= 0x1F;   
    i = bg_y>>2;
    bg_x+=sx;bg_x &= 0x1F;
    j = 0x80>>(bg_x>>2);
    POKE(CHAR_ADDR+GETID(5,3)*8+i,j);
    */
    if (state == GAME_OVER) {bg_x+=2;bg_y+=2;}
    bg_x+=sx;bg_x &= 0x1F;
    j = bg_x>>2;
    bg_y+=sy;bg_y &= 0x1F;   
    i = bg_y>>2;
    memcpy(CHAR_ADDR+i+GETID(5,3)*8,CHAR_ADDR+GETID(j,4)*8,8-i);
    memcpy(CHAR_ADDR+GETID(5,3)*8,CHAR_ADDR+GETID(j,4)*8+8-i,i);
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
        if (i >= SCREEN_H*2+2) 
        {
            if (sx == 0 && sy == 0)
            {
                set_state(RUN);
                spawn = 100;
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
            wi = ((1+i)*8)<<PS;
            next_enemy_pos:
            if (enemies[k].y < wi)
            {
                enemies[k].y = wi;
            }
            if (wi > (SCREEN_H*8)<<PS)
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
        else 
        {
            if (enemies[0].x == VALUE_FREE)
            {
                enemies[0].x = SCREEN_W*4<<PS;
                enemies[0].y = SCREEN_H*8<<PS;
            }
        }
    }
    if (bx != VALUE_FREE)
    {
        SPRITEX(1,bx);
        SPRITEY(1,by);
        if (bx > (SCREEN_W-1)*8 || by > (SCREEN_H-1)*8)
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
    xxp = px>>PS;
    yyp = py>>PS;

    next_enemy:
    if ( enemies[i].x != VALUE_FREE)
    {
        xx = enemies[i].x>>PS;
        yy = enemies[i].y>>PS;
        SPRITEX(2+i,xx);
        SPRITEY(2+i,yy);
        if (enemies[i].die)
        {
            SPRITE_POINTER(2+i,5+1);
            if ((enemies[i].die&0x2)==0)
            {
                SPRITE_POINTER(2+i,5+5);
            }
            --enemies[i].die;
            if (enemies[i].die == 0)
            {
                remove_enemy:
                SPRITEX(2+i,-SPRITE_X_OFFSET);      
                enemies[i].x = VALUE_FREE;
            }
            goto skip_enemy;
        }
        j = (counter>>3)&0x3;
        SPRITE_POINTER(2+i,5+j);
        if (bx != VALUE_FREE)
        {
            swi = xx-(bx+6);
            sj = yy-(by+5);
            if (swi > -24 && swi < 8 && sj > -21 && sj < 8)
            {
                enemies[i].die = 50;
                ++score;
                SPRITE_DISABLE(1);
                bx = VALUE_FREE;
            }
        }

        swi = xx-xxp;
        sj = yy-yyp;
        if (swi > -20 && swi < 20 && sj > -21 && sj < 21)
        {
            if (state == RUN)
            {
                sx = -swi/4;
                sy = -sj/4;
                if (power)
                {
                    POKE(SCREEN_ADDR+21+power+(SCREEN_H-1)*SCREEN_W,GETID(1,5));
                    --power;
                    blink_counter = sizeof(blink2);
                }
                else 
                {
                    set_state(GAME_OVER);
                    print(16,2,1,"GAME OVER");
                    if (hiscore < score)
                    {
                        print(15,4,1,"NEW HISCORE");
                    }
                }
            }
        }
        j = enemies[i].type;
        if (j == TYPE_FOLLOW)
        {
            if ((counter&0x3) == 0)
            {
                if (xxp < xx) 
                {
                    if (enemies[i].sx > -MAX_ENEMY_SPEED) --enemies[i].sx;
                }
                else  
                {
                    if (enemies[i].sx < MAX_ENEMY_SPEED) ++enemies[i].sx;
                }
                if (yyp < yy) 
                {
                    if (enemies[i].sy > -MAX_ENEMY_SPEED) --enemies[i].sy;
                }
                else  
                {
                    if (enemies[i].sy < MAX_ENEMY_SPEED) ++enemies[i].sy;
                }
            }
        }
        else if (j == TYPE_UP || j == TYPE_DOWN)
        {
            if (j == TYPE_UP)
            {
                if (enemies[i].sy > -MAX_ENEMY_SPEED) --enemies[i].sy;
                if (enemies[i].y+SPRITE_Y_OFFSET > 0xff00) 
                {
                    goto remove_enemy;
                }
            }
            else 
            {
                if (enemies[i].sy < MAX_ENEMY_SPEED) ++enemies[i].sy;
                if (enemies[i].y > (SCREEN_H*8<<PS) && enemies[i].y < (SCREEN_H*2*8<<PS)) 
                {
                    goto remove_enemy;
                }
            }
            if ((counter&0x3) == 0)
            {
                if (xxp < xx) 
                {
                    if (enemies[i].sx > -MAX_ENEMY_SPEED) --enemies[i].sx;
                }
                else  
                {
                    if (enemies[i].sx < MAX_ENEMY_SPEED) ++enemies[i].sx;
                }
            }
        }
        else if (j == TYPE_LEFT || j == TYPE_RIGHT)
        {
            if (j == TYPE_LEFT)
            {
                if (enemies[i].sx > -MAX_ENEMY_SPEED) --enemies[i].sx;
                if (enemies[i].x+SPRITE_Y_OFFSET > 0xff00) 
                {
                    goto remove_enemy;
                }
            }
            else 
            {
                if (enemies[i].sx < MAX_ENEMY_SPEED) ++enemies[i].sx;
                if (enemies[i].x > (SCREEN_W*8<<PS)) 
                {
                    goto remove_enemy;
                }
            }
            if ((counter&0x3) == 0)
            {
                if (yyp < yy) 
                {
                    if (enemies[i].sy > -MAX_ENEMY_SPEED) --enemies[i].sy;
                }
                else  
                {
                    if (enemies[i].sy < MAX_ENEMY_SPEED) ++enemies[i].sy;
                }
            }
        }
        enemies[i].x += enemies[i].sx;
        enemies[i].y += enemies[i].sy;
    }
    skip_enemy:
    ++i; if (i != MAX_ENEMIES) goto next_enemy;

    if (spawn)
    {
        --spawn;
        if (spawn == 0)
        {
            next_level:
            cptr = level[level_counter];
            //print_number(10,1,level_counter);
            if (cptr)
            {
                //print_number(10,2,pattern_counter);
                if (cptr[pattern_counter] == 0) 
                {
                    ++level_counter;
                    pattern_counter = 0;
                    goto next_level;
                }
                i = 0;
                check_next_enemy:
                if (enemies[i].x == VALUE_FREE)
                {
                    //print_number(1,1,cptr[pattern_counter]*8);
                    enemies[i].x = cptr[pattern_counter]*8<<PS;++pattern_counter;
                    //print_number(1,2,cptr[pattern_counter]*8);
                    enemies[i].y = cptr[pattern_counter]*8<<PS;++pattern_counter;
                    //print_number(1,3,cptr[pattern_counter]);
                    enemies[i].type = cptr[pattern_counter];++pattern_counter;
                    enemies[i].die = 0;
                    //print_number(1,4,cptr[pattern_counter]);
                    spawn = cptr[pattern_counter]*speed;++pattern_counter;
                }
                else 
                {
                    ++i; if (i != MAX_ENEMIES) goto check_next_enemy;
                    spawn = 100;
                }
            } 
            else 
            {
                level_counter = 0;
                if (speed >= 20) speed -= 10; else speed = 5;
                goto next_level;
            }
        }
    }
    if (show_score != score)
    {
        if (show_score < score) ++show_score; else --show_score;
        print_number_word(9,SCREEN_H-1,show_score);
    }
    if (show_hiscore != hiscore)
    {
        if (show_hiscore < hiscore) ++show_hiscore; else --show_hiscore;
        print_number_word(SCREEN_W-11,SCREEN_H-1,show_hiscore);
    }
    if (txt_ptr)
    {
        if ((counter&0x07) == 0)
        {
            tmp[0] = txt_ptr[txt_cursor];
            if (tmp[0])
            {
                tmp[1] = 0;
                print(15+txt_cursor,0,7,tmp);
                ++txt_cursor;
            }
            else 
            {
                txt_ptr = 0;
            }
        }
    }
    goto loop;
}