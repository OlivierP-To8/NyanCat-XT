/*******************************************************************************
* Author  : OlivierP                                                           *
* Date    : october & november 2012                                            *
* License : GNU GPLv3 (http://www.gnu.org/copyleft/gpl.html)                   *
*******************************************************************************/

#ifndef SNDBLAST_H
#define SNDBLAST_H

#include "dma.h"
#include "ram.h"

typedef struct
{
    RAM_BUFFER *sample;
    unsigned short samplingrate;
    char channels, bps;

    unsigned long curpos;
    char done, loop;
} SB_SAMPLE;

typedef struct SB_CARD_
{
    int baseio;
    int mixeraddr;
    int mixerdata;
    int resetport;
    int readport;
    int writeport;
    int pollport;
    int poll16port;

    char irq, dma8, dma16;
    int dspversion;

    int support16bits, supportStereo, supportDmaAuto;
    unsigned short supportMinMono, supportMaxMono, supportMinStereo, supportMaxStereo;

    int (*init)(struct SB_CARD_ *sb_card, SB_SAMPLE *sb_sample, DMA_BUFFER *dma_buffer);
    void (*play)();
    void (*stop)();
} SB_CARD;

SB_CARD *createSBCard(int forcedspversion);

int testCardCapabilities(SB_CARD *sb_card, SB_SAMPLE *sb_sample);

#endif
