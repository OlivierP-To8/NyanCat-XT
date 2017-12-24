/*******************************************************************************
* Author  : OlivierP                                                           *
* Date    : october & november 2012                                            *
* License : GNU GPLv3 (http://www.gnu.org/copyleft/gpl.html)                   *
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <dos.h>
#include "dma.h"

#ifdef __WATCOMC__
#include <malloc.h>
#define outportb outp
#define inportb inp
#else
/* __TURBOC__ */
#include <alloc.h>
#endif

#define lo(value) (unsigned char)( value & 0x00FF )
#define hi(value) (unsigned char)((value & 0xFF00) >> 8)

unsigned long getLinearAddr(void *p)
{
    unsigned long addr = (unsigned long)FP_SEG(p);
    addr <<= 4;
    addr += (unsigned long)FP_OFF(p);
    return addr;
}

unsigned long (*getOffset)(unsigned long p);

unsigned long getOffset16b(unsigned long p)
{
    unsigned long ofs = ((p >> 1) & 0xFFFF);
    return ofs;
}

unsigned long getOffset8b(unsigned long p)
{
    unsigned long ofs = (p & 0xFFFF);
    return ofs;
}

int getDmaBuffer(DMA_BUFFER *buf_dma)
{
    /* Find a block of memory that does not cross a page boundary */
    if ((buf_dma->buf_mem = malloc(2*buf_dma->buf_size)) != NULL)
    {
        if (getOffset(getLinearAddr(buf_dma->buf_mem)) + 2*buf_dma->buf_size > 0x10000)
        {
            void far *new_buf_mem = malloc(2*buf_dma->buf_size);
            if (new_buf_mem != NULL)
            {
                free(buf_dma->buf_mem);
                buf_dma->buf_mem = new_buf_mem; /* avoid crossing boundary */
            }
        }

        buf_dma->buf_part[0] = (unsigned char far *)buf_dma->buf_mem;
        buf_dma->buf_part[1] = buf_dma->buf_part[0] + buf_dma->buf_size;
        buf_dma->buf_addr = getLinearAddr(buf_dma->buf_mem);
        buf_dma->buf_page = buf_dma->buf_addr >> 16;
        buf_dma->buf_ofs  = getOffset(buf_dma->buf_addr);
#ifdef DEBUG
        {
            FILE *debug = fopen("debug.txt", "a+");
            if (debug != NULL)
            {
                fprintf(debug, "DMA buffer 0 = [0x%lx 0x%lx]\n", (unsigned long)(buf_dma->buf_part[0]), (unsigned long)(buf_dma->buf_part[0])+buf_dma->buf_size);
                fprintf(debug, "DMA buffer 1 = [0x%lx 0x%lx]\n", (unsigned long)(buf_dma->buf_part[1]), (unsigned long)(buf_dma->buf_part[1])+buf_dma->buf_size);
                fclose(debug);
            }
        }
#endif
    }
    return (buf_dma->buf_mem != NULL);
}

void freeDmaBuffer(DMA_BUFFER *buf_dma)
{
    free(buf_dma->buf_mem);
    buf_dma->buf_mem = NULL;
}

void setDmaTransfer16bLength(DMA_BUFFER *buf_dma, char dma_ch, DMA_MODE dma_mode, unsigned long buf_length)
{
    unsigned char dma_pageport;
    unsigned char dma_baseaddrport = ((dma_ch & 3) << 2) + 0xC0;
    unsigned char dma_countport = ((dma_ch & 3) << 2) + 0xC2;

    buf_dma->dma_stopmask  = (dma_ch & 3) | 0x04;
    buf_dma->dma_startmask = dma_ch & 3;

    switch (dma_ch)
    {
    case 4:
        dma_pageport=0x8F;
        break;
    case 5:
        dma_pageport=0x8B;
        break;
    case 6:
        dma_pageport=0x89;
        break;
    case 7:
        dma_pageport=0x8A;
        break;
    }

    outportb(0xD4, buf_dma->dma_stopmask);
    outportb(0xD6, buf_dma->dma_startmask | dma_mode);
    outportb(0xD8, 0x00);
    outportb(dma_baseaddrport, lo(buf_dma->buf_ofs));
    outportb(dma_baseaddrport, hi(buf_dma->buf_ofs));
    outportb(dma_pageport,     buf_dma->buf_page);
    outportb(dma_countport,    lo(buf_length-1)); /* number of words */
    outportb(dma_countport,    hi(buf_length-1));
    outportb(0xD4, buf_dma->dma_startmask);
}

void setDmaTransfer16b(DMA_BUFFER *buf_dma, char dma_ch, DMA_MODE dma_mode)
{
    setDmaTransfer16bLength(buf_dma, dma_ch, dma_mode, 2*buf_dma->sample_size);
}

void setDmaTransfer8bLength(DMA_BUFFER *buf_dma, char dma_ch, DMA_MODE dma_mode, unsigned long buf_length)
{
    unsigned char dma_pageport;
    unsigned char dma_baseaddrport = ((dma_ch & 3) << 1) + 0x00;
    unsigned char dma_countport = ((dma_ch & 3) << 1) + 0x01;

    buf_dma->dma_stopmask  = dma_ch | 0x04;
    buf_dma->dma_startmask = dma_ch;

    switch (dma_ch)
    {
    case 0:
        dma_pageport=0x87;
        break;
    case 1:
        dma_pageport=0x83;
        break;
    case 2:
        dma_pageport=0x81;
        break;
    case 3:
        dma_pageport=0x82;
        break;
    }

    outportb(0x0A, buf_dma->dma_stopmask);
    outportb(0x0B, buf_dma->dma_startmask | dma_mode);
    outportb(0x0C, 0x00);
    outportb(dma_baseaddrport, lo(buf_dma->buf_ofs));
    outportb(dma_baseaddrport, hi(buf_dma->buf_ofs));
    outportb(dma_pageport,     buf_dma->buf_page);
    outportb(dma_countport,    lo(buf_length-1)); /* number of bytes */
    outportb(dma_countport,    hi(buf_length-1));
    outportb(0x0A, buf_dma->dma_startmask);
}

void setDmaTransfer8b(DMA_BUFFER *buf_dma, char dma_ch, DMA_MODE dma_mode)
{
    setDmaTransfer8bLength(buf_dma, dma_ch, dma_mode, 2*buf_dma->sample_size);
}

void stopDmaTransfer16b(DMA_BUFFER *buf_dma)
{
    outportb(0xD4, buf_dma->dma_stopmask);
}

void stopDmaTransfer8b(DMA_BUFFER *buf_dma)
{
    outportb(0x0A, buf_dma->dma_stopmask);
}

DMA_BUFFER *createDmaTransfer(unsigned short size, char bps)
{
    DMA_BUFFER *buf_dma = malloc(sizeof(DMA_BUFFER));
    if (buf_dma != NULL)
    {
        buf_dma->buf_size = size;
        buf_dma->sample_size = size / bps;
        buf_dma->curbuf = 0;
        buf_dma->getDmaBuffer = getDmaBuffer;
        buf_dma->freeDmaBuffer = freeDmaBuffer;
        if (bps==1)
        {
            getOffset = getOffset8b;
            buf_dma->setDmaTransfer = setDmaTransfer8b;
            buf_dma->setDmaTransferLength = setDmaTransfer8bLength;
            buf_dma->stopDmaTransfer = stopDmaTransfer8b;
        }
        else
        {
            getOffset = getOffset16b;
            buf_dma->setDmaTransfer = setDmaTransfer16b;
            buf_dma->setDmaTransferLength = setDmaTransfer16bLength;
            buf_dma->stopDmaTransfer = stopDmaTransfer16b;
        }
        if (!buf_dma->getDmaBuffer(buf_dma))
        {
            free(buf_dma);
            buf_dma = NULL;
        }
    }
    return buf_dma;
}
