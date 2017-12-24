/*******************************************************************************
* Author  : OlivierP                                                           *
* Date    : march & july 2012                                                  *
* License : GNU GPLv3 (http://www.gnu.org/copyleft/gpl.html)                   *
*******************************************************************************/

#ifndef SCREEN_H
#define SCREEN_H

#define ENDL 0xEE

typedef enum
{
    /* order does matter, the first device must be compatible with the other ones */
    vVGAH,
    vVGA,
    vEGAH,
    vEGA,
    vPlantronicsColorPlus,
    vNULL
} VIDEODEVICE;

typedef struct
{
    unsigned char *SpriteAdr;
    unsigned short MemAdr;
} SPRITEPOS;

typedef struct SCREEN_
{
    VIDEODEVICE videoDevice;
    unsigned char cpage; /* current page */
    unsigned short vpage; /* byte offset to current screen (0 or pagesize) */
    unsigned short pagesize; /* size of a screen in byte */
    unsigned short needRefresh; /* 1 to allow change page */
    void (*SetPalette)(const unsigned char *palette, const int nbc);
    void (*SetVideoMode)(const int mode);
    void (*ChangeVideoPage)(struct SCREEN_ *scr);
    void (*EraseScreen)(struct SCREEN_ *scr, unsigned char color);
    void (*PrintSprite)(const struct SCREEN_ *scr, const SPRITEPOS *sprite);
    void (*PrintSpriteTr)(const struct SCREEN_ *scr, const SPRITEPOS *sprite, unsigned char alpha);
} SCREEN;

void Screen_SetPalette16c(const unsigned char *palette, const int nbc);
void Screen_SetPalette256c(const unsigned char *palette, const int nbc);
void Screen_SetVideoMode(const int mode);
void Screen_SetGraphicMode(SCREEN *scr);
void Screen_ChangeVideoPage(SCREEN *scr);

SCREEN *CreateScreen(VIDEODEVICE device);
SCREEN *DeleteScreen(SCREEN *scr);

#endif
