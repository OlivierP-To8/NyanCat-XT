/*******************************************************************************
* Author  : OlivierP                                                           *
* Date    : march & july 2012                                                  *
* License : GNU GPLv3 (http://www.gnu.org/copyleft/gpl.html)                   *
*******************************************************************************/

#include <stdio.h>
#include <dos.h> /* MK_FP */
#include <conio.h> /* outportb */
#include "screen.h"

#ifdef __WATCOMC__
#include <memory.h>
#include <malloc.h>
#define outportb outp
#define inportb inp
#else
/* __TURBOC__ */
#include <mem.h>
#include <alloc.h>
#endif

#define lo(value) (unsigned char)( value & 0x00FF )
#define hi(value) (unsigned char)((value & 0xFF00) >> 8)

/*
   http://webpages.charter.net/danrollins/techhelp/0067.HTM
   http://www.faqs.org/faqs/msdos-programmer-faq/part4/section-5.html
*/
VIDEODEVICE Screen_DetectVideo()
{
    union REGS regs;
    VIDEODEVICE retval = vNULL;

    regs.x.ax = 0x1200;
    regs.x.bx = 0x0032;
    int86(0x10, &regs, &regs);
    if (regs.h.al == 0x12)
    {
        retval = vVGAH;
    }
    else
    {
        regs.x.ax = 0x1200;
        regs.x.bx = 0x0010;
        int86(0x10, &regs, &regs);
        if (regs.h.bl < 4)
        {
            if (regs.h.bl == 3)
                retval = vEGAH;
            else
                retval = vEGA;
        }
        else
        {
            retval = vPlantronicsColorPlus;
        }
    }

    return retval;
}

void Screen_SetVideoMode(const int mode)
{
    union REGS regs;

    regs.x.ax = mode;
    int86(0x10, &regs, &regs);
}

/*
   http://www.seasip.info/VintagePC/cga.html
   http://wiki.osdev.org/VGA_Hardware
*/
void Screen_SetGraphicMode(SCREEN *scr)
{
    switch (scr->videoDevice)
    {
    case vPlantronicsColorPlus:
    {
        Screen_SetVideoMode(0x0004);

        /* Bit 4: 320x200 16-colour mode */
        outportb(0x3dd, 0x10);
        break;
    }

    case vEGA:
        Screen_SetVideoMode(0x000d);
        break;

    case vEGAH:
        Screen_SetVideoMode(0x0010);
        break;

    case vVGAH:
    {
        unsigned char data;

        Screen_SetVideoMode(0x0010);

        /* http://www.phatcode.net/res/224/files/html/ch45/45-04.html */
        /* Modify the sync polarity bits (bits 7 & 6) of the
           Miscellaneous Output register (readable at 0x3CC, writable at
           0x3C2) to select the 400-scan-line vertical scanning rate */
        data = inportb(0x3cc);
        outportb(0x3c2, (data & 0x3f) | 0x40);

        /* Now, tweak the registers needed to convert the vertical
           timings from 350 to 400 scan lines */
        outportb(0x3d4, 0x10); /* Vertical Sync Start register */
        outportb(0x3d5, 0x9c);

        outportb(0x3d4, 0x11); /* Vertical Sync End register */
        outportb(0x3d5, 0x8e);

        outportb(0x3d4, 0x12); /* Vertical Display End register */
        outportb(0x3d5, 0x8f);

        outportb(0x3d4, 0x15); /* Vertical Blank Start register */
        outportb(0x3d5, 0x96);

        outportb(0x3d4, 0x16); /* Vertical Blank End register */
        outportb(0x3d5, 0xb9);
        break;
    }

    case vVGA:
    {
        unsigned char data;

        Screen_SetVideoMode(0x0013);

        /* turn off chain-4 mode */
        outportb(0x03c4, 0x04);
        data = inportb(0x03c5);
        outportb(0x03c5, (data & 0xf7) | 0x04);

        /* turn off doubleword clocking */
        outportb(0x03d4, 0x14);
        data = inportb(0x03d5);
        outportb(0x03d5, data & 0xbf);

        /* turn off word clocking */
        outportb(0x03d4, 0x17);
        data = inportb(0x03d5);
        outportb(0x03d5, data | 0x40);

        break;
    }
    }
}

/* http://en.wikipedia.org/wiki/Talk%3AMode_X */
void Screen_ChangeVideoPage(SCREEN *scr)
{
    /* attente du timer FPS */
    while (scr->needRefresh == 0)
    {
        ;
    }
    scr->needRefresh = 0;

    if (scr->videoDevice == vPlantronicsColorPlus)
        return;

    /* wait for display disable */
    while (inportb(0x3da) & 0x01);

    /* switch page */
    outportb(0x3d4, 0x0c);
    outportb(0x3d5, hi(scr->vpage));
    outportb(0x3d4, 0x0d);
    outportb(0x3d5, lo(scr->vpage));

    if (scr->cpage == 0)
    {
        scr->cpage = 1;
        scr->vpage = scr->pagesize;
    }
    else
    {
        scr->cpage = 0;
        scr->vpage = 0;
    }

    /* wait until we are in vertical sync */
    while (!(inportb(0x3da) & 0x08));
}

/****************************************************************************/
/******************************** PALETTE ***********************************/
/****************************************************************************/
/*
   http://webpages.charter.net/danrollins/techhelp/0137.HTM
   http://webpages.charter.net/danrollins/techhelp/0139.HTM
*/
void Screen_SetPalette16c(const unsigned char *palette, const int nbc)
{
    union REGS regs;
    struct SREGS sregs;

    regs.x.ax = 0x1002;
    regs.x.dx = FP_OFF(palette);
    sregs.es = FP_SEG(palette);
    int86x(0x10, &regs, &regs, &sregs);
}

void Screen_SetPalette256c(const unsigned char *palette, const int nbc)
{
    int j;
    outportb(0x03c6, 0xff); /* mask all registers */
    outportb(0x03c8, 0);
    for (j=0; j<nbc*3; j++)
    {
        outportb(0x03c9, palette[j]>>2); /* 6 bit value */
    }
}

/****************************************************************************/
/******************************** EraseScreen *******************************/
/****************************************************************************/

void Screen_EraseScreenCGA(SCREEN *scr, unsigned char color)
{
    int i;
    unsigned char bRG, bBI, pRG=0, pBI=0;
    char far *screenEven  = (char far *)MK_FP(0xb800, 0);
    char far *screenOdd   = (char far *)MK_FP(0xba00, 0);
    char far *screenEven2 = (char far *)MK_FP(0xbc00, 0);
    char far *screenOdd2  = (char far *)MK_FP(0xbe00, 0);

    bRG = (color & 0x0C) >> 2;
    bBI =  color & 0x03;
    for (i=0; i<4; i++)
    {
        unsigned char n = 6-(2*i);
        pRG |= bRG << n;
        pBI |= bBI << n;
    }

    memset(screenEven, pRG, scr->pagesize);
    memset(screenEven2, pBI, scr->pagesize);
    memset(screenOdd, pRG, scr->pagesize);
    memset(screenOdd2, pBI, scr->pagesize);

    scr->needRefresh = 1;
}

/* http://webpages.charter.net/danrollins/techhelp/0089.HTM */
void Screen_EraseScreenEGA(SCREEN *scr, unsigned char color)
{
    int i;
    unsigned char b0, b1, b2, b3;
    unsigned char p0=0, p1=0, p2=0, p3=0;
    char far *screen = (char far *)MK_FP(0xa000, scr->vpage);

    b0 =  color & 0x01;
    b1 = (color & 0x02) >> 1;
    b2 = (color & 0x04) >> 2;
    b3 = (color & 0x08) >> 3;
    for (i=0; i<8; i++)
    {
        unsigned char n = 7-i;
        p3 |= b3 << n;
        p2 |= b2 << n;
        p1 |= b1 << n;
        p0 |= b0 << n;
    }

    outportb(0x03c4, 0x02);
    outportb(0x03c5, 0x01);
    memset(screen, p0, scr->pagesize);
    outportb(0x03c5, 0x02);
    memset(screen, p1, scr->pagesize);
    outportb(0x03c5, 0x04);
    memset(screen, p2, scr->pagesize);
    outportb(0x03c5, 0x08);
    memset(screen, p3, scr->pagesize);

    scr->needRefresh = 1;
}

void Screen_EraseScreenVGA(SCREEN *scr, unsigned char color)
{
    char far *screen = (char far *)MK_FP(0xa000, scr->vpage);

    /* erase 4 planes at once */
    outportb(0x03c4, 0x02);
    outportb(0x03c5, 0x0f);
    memset(screen, color, scr->pagesize);
    scr->needRefresh = 1;
}

/****************************************************************************/
/******************************** PrintSprite *******************************/
/****************************************************************************/

void Screen_PrintSpriteCGA(const SCREEN *scr, const SPRITEPOS *sprite)
{
    if (sprite != NULL)
    {
        int pos = 0, stop = 0, even = 1;
        unsigned char p0, p1;
        unsigned char *data = sprite->SpriteAdr;
        char far *screenEven  = (char far *)MK_FP(0xb800, sprite->MemAdr);
        char far *screenOdd   = (char far *)MK_FP(0xba00, sprite->MemAdr);
        char far *screenEven2 = (char far *)MK_FP(0xbc00, sprite->MemAdr);
        char far *screenOdd2  = (char far *)MK_FP(0xbe00, sprite->MemAdr);

        while (stop == 0)
        {
            p0 = data[pos++];
            p1 = data[pos++];
            if (even)
            {
                *screenEven = p0;
                screenEven++;
                *screenEven2 = p1;
                screenEven2++;
            }
            else
            {
                *screenOdd = p0;
                screenOdd++;
                *screenOdd2 = p1;
                screenOdd2++;
            }

            if (data[pos] == ENDL)
            {
                unsigned char delta = data[++pos];
                pos++;
                if (delta == 0)
                    stop = 1;
                if (even)
                {
                    even = 0;
                    screenEven += delta;
                    screenEven2 += delta;
                }
                else
                {
                    even = 1;
                    screenOdd += delta;
                    screenOdd2 += delta;
                }
            }
        }
    }
}

void Screen_PrintSpriteTrCGA(const SCREEN *scr, const SPRITEPOS *sprite, unsigned char alpha)
{
    if (sprite != NULL)
    {
        int pos = 0, stop = 0, even = 1, i;
        unsigned char p0, p1, values[4];
        unsigned char *data = sprite->SpriteAdr;
        char far *screenEven  = (char far *)MK_FP(0xb800, sprite->MemAdr);
        char far *screenOdd   = (char far *)MK_FP(0xba00, sprite->MemAdr);
        char far *screenEven2 = (char far *)MK_FP(0xbc00, sprite->MemAdr);
        char far *screenOdd2  = (char far *)MK_FP(0xbe00, sprite->MemAdr);

        while (stop == 0)
        {
            p0 = data[pos++];
            p1 = data[pos++];

            values[0] = ((p0 & 0xc0) >> 4) | ((p1 & 0xc0) >> 6);
            values[1] = ((p0 & 0x30) >> 2) | ((p1 & 0x30) >> 4);
            values[2] =  (p0 & 0x0c)       | ((p1 & 0x0c) >> 2);
            values[3] = ((p0 & 0x03) << 2) |  (p1 & 0x03);

            /* reading pixels from video memory */
            if (even)
            {
                p0 = *screenEven;
                p1 = *screenEven2;
            }
            else
            {
                p0 = *screenOdd;
                p1 = *screenOdd2;
            }

            /* replace pixels with video memory if needed */
            if (values[0] == alpha)
                values[0] = ((p0 & 0xc0) >> 4) | ((p1 & 0xc0) >> 6);
            if (values[1] == alpha)
                values[1] = ((p0 & 0x30) >> 2) | ((p1 & 0x30) >> 4);
            if (values[2] == alpha)
                values[2] =  (p0 & 0x0c)       | ((p1 & 0x0c) >> 2);
            if (values[3] == alpha)
                values[3] = ((p0 & 0x03) << 2) |  (p1 & 0x03);

            p0 = p1 = 0;
            for (i=0; i<4; i++)
            {
                unsigned char n = 6-(2*i);
                p0 |= ((values[i] & 0x0c) >> 2) << n;
                p1 |=  (values[i] & 0x03)       << n;
            }

            if (even)
            {
                *screenEven = p0;
                screenEven++;
                *screenEven2 = p1;
                screenEven2++;
            }
            else
            {
                *screenOdd = p0;
                screenOdd++;
                *screenOdd2 = p1;
                screenOdd2++;
            }

            if (data[pos] == ENDL)
            {
                unsigned char delta = data[++pos];
                pos++;
                if (delta == 0)
                    stop = 1;
                if (even)
                {
                    even = 0;
                    screenEven += delta;
                    screenEven2 += delta;
                }
                else
                {
                    even = 1;
                    screenOdd += delta;
                    screenOdd2 += delta;
                }
            }
        }
    }
}

void Screen_PrintSpritePlanar(const SCREEN *scr, const SPRITEPOS *sprite)
{
    if (sprite != NULL)
    {
        int pos = 0, stop = 0;
        unsigned char *data = sprite->SpriteAdr;
        char far *screen = (char far *)MK_FP(0xa000, scr->vpage + sprite->MemAdr);

        outportb(0x03c4, 0x02);
        while (stop == 0)
        {
            outportb(0x03c5, 0x01);
            *screen = data[pos++];

            outportb(0x03c5, 0x02);
            *screen = data[pos++];

            outportb(0x03c5, 0x04);
            *screen = data[pos++];

            outportb(0x03c5, 0x08);
            *screen = data[pos++];

            screen++;
            if (data[pos] == ENDL)
            {
                unsigned char delta = data[++pos];
                pos++;
                if (delta == 0)
                    stop = 1;
                screen += delta;
            }
        }
    }
}

void Screen_PrintSpriteTr256cPlanar(const SCREEN *scr, const SPRITEPOS *sprite, unsigned char alpha)
{
    if (sprite != NULL)
    {
        int pos = 0, stop = 0;
        unsigned char pixel;
        unsigned char *data = sprite->SpriteAdr;
        char far *screen = (char far *)MK_FP(0xa000, scr->vpage + sprite->MemAdr);

        outportb(0x03c4, 0x02);
        while (stop == 0)
        {
            pixel = data[pos++];
            if (pixel != alpha)
            {
                outportb(0x03c5, 0x01);
                *screen = pixel;
            }

            pixel = data[pos++];
            if (pixel != alpha)
            {
                outportb(0x03c5, 0x02);
                *screen = pixel;
            }

            pixel = data[pos++];
            if (pixel != alpha)
            {
                outportb(0x03c5, 0x04);
                *screen = pixel;
            }

            pixel = data[pos++];
            if (pixel != alpha)
            {
                outportb(0x03c5, 0x08);
                *screen = pixel;
            }

            screen++;
            if (data[pos] == ENDL)
            {
                unsigned char delta = data[++pos];
                pos++;
                if (delta == 0)
                    stop = 1;
                screen += delta;
            }
        }
    }
}

void Screen_PrintSpriteTr16cPlanar(const SCREEN *scr, const SPRITEPOS *sprite, unsigned char alpha)
{
    if (sprite != NULL)
    {
        int pos = 0, stop = 0, i;
        unsigned char p0, p1, p2, p3, values[8];
        unsigned char *data = sprite->SpriteAdr;
        char far *screen = (char far *)MK_FP(0xa000, scr->vpage + sprite->MemAdr);

        while (stop == 0)
        {
            p0 = data[pos++];
            p1 = data[pos++];
            p2 = data[pos++];
            p3 = data[pos++];

            /* compute pixels */
            values[0] = (p3 & 0x80) >> 4 | (p2 & 0x80) >> 5 | (p1 & 0x80) >> 6 | (p0 & 0x80) >> 7;
            values[1] = (p3 & 0x40) >> 3 | (p2 & 0x40) >> 4 | (p1 & 0x40) >> 5 | (p0 & 0x40) >> 6;
            values[2] = (p3 & 0x20) >> 2 | (p2 & 0x20) >> 3 | (p1 & 0x20) >> 4 | (p0 & 0x20) >> 5;
            values[3] = (p3 & 0x10) >> 1 | (p2 & 0x10) >> 2 | (p1 & 0x10) >> 3 | (p0 & 0x10) >> 4;
            values[4] = (p3 & 0x08)      | (p2 & 0x08) >> 1 | (p1 & 0x08) >> 2 | (p0 & 0x08) >> 3;
            values[5] = (p3 & 0x04) << 1 | (p2 & 0x04)      | (p1 & 0x04) >> 1 | (p0 & 0x04) >> 2;
            values[6] = (p3 & 0x02) << 2 | (p2 & 0x02) << 1 | (p1 & 0x02)      | (p0 & 0x02) >> 1;
            values[7] = (p3 & 0x01) << 3 | (p2 & 0x01) << 2 | (p1 & 0x01) << 1 | (p0 & 0x01)     ;

            /* reading pixels from video memory */
            outportb(0x03ce, 0x04);
            outportb(0x03cf, 0x00);
            p0 = *screen;
            outportb(0x03cf, 0x01);
            p1 = *screen;
            outportb(0x03cf, 0x02);
            p2 = *screen;
            outportb(0x03cf, 0x03);
            p3 = *screen;

            /* replace pixels with video memory if needed */
            if (values[0] == alpha)
                values[0] = (p3 & 0x80) >> 4 | (p2 & 0x80) >> 5 | (p1 & 0x80) >> 6 | (p0 & 0x80) >> 7;
            if (values[1] == alpha)
                values[1] = (p3 & 0x40) >> 3 | (p2 & 0x40) >> 4 | (p1 & 0x40) >> 5 | (p0 & 0x40) >> 6;
            if (values[2] == alpha)
                values[2] = (p3 & 0x20) >> 2 | (p2 & 0x20) >> 3 | (p1 & 0x20) >> 4 | (p0 & 0x20) >> 5;
            if (values[3] == alpha)
                values[3] = (p3 & 0x10) >> 1 | (p2 & 0x10) >> 2 | (p1 & 0x10) >> 3 | (p0 & 0x10) >> 4;
            if (values[4] == alpha)
                values[4] = (p3 & 0x08)      | (p2 & 0x08) >> 1 | (p1 & 0x08) >> 2 | (p0 & 0x08) >> 3;
            if (values[5] == alpha)
                values[5] = (p3 & 0x04) << 1 | (p2 & 0x04)      | (p1 & 0x04) >> 1 | (p0 & 0x04) >> 2;
            if (values[6] == alpha)
                values[6] = (p3 & 0x02) << 2 | (p2 & 0x02) << 1 | (p1 & 0x02)      | (p0 & 0x02) >> 1;
            if (values[7] == alpha)
                values[7] = (p3 & 0x01) << 3 | (p2 & 0x01) << 2 | (p1 & 0x01) << 1 | (p0 & 0x01)     ;

            p0 = p1 = p2 = p3 = 0;
            for (i=0; i<8; i++)
            {
                unsigned char n = 7-i;
                p3 |= ((values[i] & 8) >> 3) << n;
                p2 |= ((values[i] & 4) >> 2) << n;
                p1 |= ((values[i] & 2) >> 1) << n;
                p0 |=  (values[i] & 1)       << n;
            }

            outportb(0x03c4, 0x02);
            outportb(0x03c5, 0x01);
            *screen = p0;
            outportb(0x03c5, 0x02);
            *screen = p1;
            outportb(0x03c5, 0x04);
            *screen = p2;
            outportb(0x03c5, 0x08);
            *screen = p3;

            screen++;
            if (data[pos] == ENDL)
            {
                unsigned char delta = data[++pos];
                pos++;
                if (delta == 0)
                    stop = 1;
                screen += delta;
            }
        }
    }
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

SCREEN *CreateScreen(VIDEODEVICE device)
{
    SCREEN *scr = NULL;
    VIDEODEVICE available = Screen_DetectVideo();

    if (available != vNULL)
        scr = malloc(sizeof(SCREEN));

    if (scr != NULL)
    {
        scr->cpage = 0;
        scr->vpage = 0;
        scr->SetVideoMode = Screen_SetVideoMode;
        scr->ChangeVideoPage = Screen_ChangeVideoPage;

        scr->videoDevice = available;
        if ((device != vNULL) && (scr->videoDevice <= device))
            scr->videoDevice = device;
        switch (scr->videoDevice)
        {
        case vPlantronicsColorPlus:
            scr->pagesize = 8000;
            scr->SetPalette = Screen_SetPalette16c;
            scr->EraseScreen = Screen_EraseScreenCGA;
            scr->PrintSprite = Screen_PrintSpriteCGA;
            scr->PrintSpriteTr = Screen_PrintSpriteTrCGA;
            break;
        case vEGA:
            scr->pagesize = 8000;
            scr->SetPalette = Screen_SetPalette16c;
            scr->EraseScreen = Screen_EraseScreenEGA;
            scr->PrintSprite = Screen_PrintSpritePlanar;
            scr->PrintSpriteTr = Screen_PrintSpriteTr16cPlanar;
            break;
        case vEGAH:
            scr->pagesize = 28000;
            scr->SetPalette = Screen_SetPalette16c;
            scr->EraseScreen = Screen_EraseScreenEGA;
            scr->PrintSprite = Screen_PrintSpritePlanar;
            scr->PrintSpriteTr = Screen_PrintSpriteTr16cPlanar;
            break;
        case vVGAH:
            scr->pagesize = 32000;
            scr->SetPalette = Screen_SetPalette16c;
            scr->EraseScreen = Screen_EraseScreenEGA;
            scr->PrintSprite = Screen_PrintSpritePlanar;
            scr->PrintSpriteTr = Screen_PrintSpriteTr16cPlanar;
            break;
        case vVGA:
            scr->pagesize = 16000;
            scr->SetPalette = Screen_SetPalette256c;
            scr->EraseScreen = Screen_EraseScreenVGA;
            scr->PrintSprite = Screen_PrintSpritePlanar;
            scr->PrintSpriteTr = Screen_PrintSpriteTr256cPlanar;
            break;
        }
        scr->needRefresh = 1;

        Screen_ChangeVideoPage(scr);
    }

    return scr;
}

SCREEN *DeleteScreen(SCREEN *scr)
{
    if (scr != NULL)
    {
        free(scr);
    }
    return NULL;
}
