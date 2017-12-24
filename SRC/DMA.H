/*******************************************************************************
* Author  : OlivierP                                                           *
* Date    : october & november 2012                                            *
* License : GNU GPLv3 (http://www.gnu.org/copyleft/gpl.html)                   *
*******************************************************************************/

#ifndef DMA_H
#define DMA_H

#define TRUE  1
#define FALSE 0

#include <mem.h>

typedef enum
{
    DmaModeWriteTransferSingle = 0x48,
    DmaModeWriteTransferAuto = 0x58
} DMA_MODE;

typedef struct DMA_BUFFER_
{
    void far *buf_mem;
    unsigned char far *buf_part[2];
    unsigned long buf_addr;
    unsigned char buf_page;
    unsigned long buf_ofs;
    unsigned long buf_size;
    unsigned long sample_size;
    unsigned char curbuf;
    unsigned char dma_startmask;
    unsigned char dma_stopmask;

    int (*getDmaBuffer)(struct DMA_BUFFER_ *buf_dma);
    void (*freeDmaBuffer)(struct DMA_BUFFER_ *buf_dma);
    void (*setDmaTransfer)(struct DMA_BUFFER_ *buf_dma, char dma_ch, DMA_MODE dma_mode);
    void (*setDmaTransferLength)(struct DMA_BUFFER_ *buf_dma, char dma_ch, DMA_MODE dma_mode, unsigned long buf_length);
    void (*stopDmaTransfer)(struct DMA_BUFFER_ *buf_dma);
} DMA_BUFFER;

DMA_BUFFER *createDmaTransfer(unsigned short size, char bps);

#endif
