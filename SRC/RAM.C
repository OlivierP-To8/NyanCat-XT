/*******************************************************************************
* Author  : OlivierP                                                           *
* Date    : october & november 2012                                            *
* License : GNU GPLv3 (http://www.gnu.org/copyleft/gpl.html)                   *
*******************************************************************************/

#include <stdio.h>
#include <dos.h>
#include "ram.h"

#ifdef __WATCOMC__
#include <malloc.h>
#define asm _asm
#define farmalloc(s) halloc(s, 1)
#define farfree hfree
#else
/* __TURBOC__ */
#include <alloc.h>
#endif

void far *xms_driver = NULL;

int xms_init()
{
    unsigned char retval = 0;

    asm mov ax, 0x4300;
    asm int 0x2f;
    asm mov [retval], al;

    if (retval == 0x80)
    {
        asm mov ax, 0x4310;
        asm int 0x2f;
        asm mov word ptr [xms_driver],bx;
        asm mov word ptr [xms_driver+2],es;
        return TRUE;
    }

    return FALSE;
}

unsigned short xms_getversion()
{
    unsigned short retval = 0;

    asm mov ah, 0x00;
    asm call [xms_driver];
    asm mov [retval], ax;

    return retval;
}

unsigned short xms_getfreemem()
{
    unsigned short retval = 0;

    asm mov ah, 0x08;
    asm call [xms_driver];
    asm mov [retval], ax;

    return retval;
}

unsigned short xms_allocate(unsigned short far *handleptr, unsigned long size)
{
    unsigned short retval = 0;
    unsigned short sizekb = (size / 1024L) + 1;

    asm mov ah, 0x09;
    asm mov dx, sizekb;
    asm call [xms_driver];
    asm les di, [handleptr];
    asm mov es:[di], dx;
    asm mov [retval], ax;

    return retval;
}

unsigned short xms_free(unsigned short handle)
{
    unsigned short retval = 0;

    asm mov ah, 0x0a;
    asm mov dx, [handle];
    asm call [xms_driver];
    asm mov [retval], ax;

    return retval;
}

typedef struct
{
    unsigned long length;
    unsigned short sourcehandle;
    unsigned long sourceoffset;
    unsigned short desthandle;
    unsigned long destoffset;
} XMS_MOVE;

XMS_MOVE xms_params;

unsigned short xms_move(XMS_MOVE far *xmsp)
{
    unsigned short retval = 0;

    asm push ds;
    asm mov ah, 0x0b;
    asm lds si, [xmsp];
    asm call [xms_driver];
    asm mov [retval], ax;
    asm pop ds;

    return retval;
}

unsigned char far *normalize(unsigned char far *p)
{
    /* alter a far pointer so that the offset is zero */
    unsigned long seg = FP_SEG(p);
    unsigned short off = FP_OFF(p);
    unsigned char far *retval;

    seg += (off / 16);
    off &= 0x000F;
    retval = MK_FP((unsigned short)seg, off);

    return retval;
}

unsigned char far *normalizeinc(unsigned char far *p, unsigned long pos)
{
    unsigned long seg = FP_SEG(p);
    unsigned short off = FP_OFF(p);
    unsigned long p2;
    unsigned char far *retval;

    seg += (off / 16);
    off &= 0x000F;
    p2 = seg << 4 | off;
    p2 += pos;
    retval = MK_FP((unsigned short)(p2 >> 4), (unsigned short)(p2 & 0x000F));

    return retval;
}

RAM_BUFFER *createRamBuffer(unsigned long size)
{
    RAM_BUFFER *buf_ram = malloc(sizeof(RAM_BUFFER));
    if (buf_ram != NULL)
    {
        buf_ram->xms_handle = 0;
        buf_ram->buf_ptr = (unsigned char far *)farmalloc(size);
        buf_ram->buf_size = size;
        if (buf_ram->buf_ptr == NULL)
        {
            if (xms_init())
            {
                unsigned short xmsversion = xms_getversion();
                printf("XMS version : 0x%x\n", xmsversion);
                if (xmsversion >= 0x0300)
                {
                    unsigned long maxsize = xms_getfreemem();
                    printf("XMS available : %lu Kb\n", maxsize);
                    if (size/1024 > maxsize)
                    {
                        size = (unsigned long)((maxsize - 1) * 1024);
                    }

                    if (xms_allocate(&(buf_ram->xms_handle), size))
                    {
                        buf_ram->buf_size = size;
                        printf("XMS free : %u Kb\n", xms_getfreemem());
                    }
                    else
                    {
                        printf("Not enough XMS !\n");
                    }
                }
            }
            if (buf_ram->xms_handle == 0)
            {
                free(buf_ram);
                buf_ram = NULL;
            }
        }
    }
    return buf_ram;
}

void copyFromRamBuffer(RAM_BUFFER *buf_ram, unsigned long pos, unsigned long size, unsigned char far *dst_ptr)
{
    if (buf_ram != NULL)
    {
        if (buf_ram->buf_ptr != NULL)
        {
            unsigned char far *src = normalizeinc(buf_ram->buf_ptr, pos);
            memcpy(dst_ptr, src, size);
        }
        else
        {
            xms_params.desthandle   = 0;
            xms_params.destoffset   = (unsigned long)(dst_ptr);
            xms_params.sourcehandle = buf_ram->xms_handle;
            xms_params.sourceoffset = pos;
            xms_params.length       = size;
            if (!xms_move(&xms_params))
                printf("XMS move error\n");
        }
    }
}

void copyToRamBuffer(RAM_BUFFER *buf_ram, unsigned long pos, unsigned long size, unsigned char far *src_ptr)
{
    if (buf_ram != NULL)
    {
        if (buf_ram->buf_ptr != NULL)
        {
            unsigned char far *dst = normalizeinc(buf_ram->buf_ptr, pos);
            memcpy(dst, src_ptr, size);
        }
        else
        {
            xms_params.sourcehandle = 0;
            xms_params.sourceoffset = (unsigned long)(src_ptr);
            xms_params.desthandle   = buf_ram->xms_handle;
            xms_params.destoffset   = pos;
            xms_params.length       = size;
            if (!xms_move(&xms_params))
                printf("XMS move error\n");
        }
    }
}

void freeRamBuffer(RAM_BUFFER *buf_ram)
{
    if (buf_ram != NULL)
    {
        if (buf_ram->buf_ptr != NULL)
        {
            farfree(buf_ram->buf_ptr);
            buf_ram->buf_ptr = NULL;
        }
        else
        {
            xms_free(buf_ram->xms_handle);
            buf_ram->xms_handle = 0;
            printf("XMS free : %u Kb\n", xms_getfreemem());
        }
    }
    free(buf_ram);
    buf_ram = NULL;
}
