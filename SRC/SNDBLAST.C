/*******************************************************************************
* Author  : OlivierP                                                           *
* Date    : october & november 2012                                            *
* License : GNU GPLv3 (http://www.gnu.org/copyleft/gpl.html)                   *
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <dos.h>
#include "sndblast.h"
#include "irq.h"

#ifdef __WATCOMC__
#include <malloc.h>
#define outportb outp
#define inportb inp
#define disable _disable
#define enable _enable
#else
/* __TURBOC__ */
#include <alloc.h>
#endif

SB_CARD *_sb_card;
SB_SAMPLE *_sb_sample;
DMA_BUFFER *_dma_buffer;

/*****************************************************************************/

void updateDmaBuffer()
{
    if (_sb_sample->curpos < _sb_sample->sample->buf_size)
    {
        unsigned long length = _sb_sample->sample->buf_size - _sb_sample->curpos;
        unsigned char far *dst_ptr = _dma_buffer->buf_part[_dma_buffer->curbuf];

        if (length > _dma_buffer->buf_size)
        {
            length = _dma_buffer->buf_size;
        }
        else
        {
            if (_sb_sample->loop == TRUE)
            {
                copyFromRamBuffer(_sb_sample->sample, _sb_sample->curpos, length, dst_ptr);
                dst_ptr += length;
                length = _dma_buffer->buf_size - length;
                _sb_sample->curpos = 0;
            }
            else
            {
                _sb_sample->done = TRUE;
            }
        }

        copyFromRamBuffer(_sb_sample->sample, _sb_sample->curpos, length, dst_ptr);
        _sb_sample->curpos += length;
        if (_sb_card->supportDmaAuto)
            _dma_buffer->curbuf ^= 1;
    }
}

/****************** interrupt auto-initialize ********************************/

void interrupt intr_handler_auto()
{
    if (_sb_sample->bps == 1)
    {
        inp(_sb_card->pollport);
    }
#if 0
    else
    {
        inp(_sb_card->poll16port);
    }
#else
    else if (_sb_card->dspversion >= 0x400)
    {
        short temp = 0;
        outp(_sb_card->mixeraddr, 0x82);
        temp = inp(_sb_card->mixerdata);
        if (temp & 1)
        {
            /* 8 bits */
            inp(_sb_card->pollport);
        }
        else if (temp & 2)
        {
            /* 16 bits */
            inp(_sb_card->poll16port);
        }
    }
#endif

    updateDmaBuffer();

    clear_intr(_sb_card->irq);
}

/****************** interrupt single-cycle ***********************************/

void start_sb_8b_mono_single();

void interrupt intr_handler_single()
{
    if (_sb_sample->curpos < _sb_sample->sample->buf_size)
    {
        updateDmaBuffer();
        _dma_buffer->setDmaTransfer(_dma_buffer, _sb_card->dma8, DmaModeWriteTransferSingle);
        start_sb_8b_mono_single();
    }

    inp(_sb_card->pollport);

    clear_intr(_sb_card->irq);
}

/****************** interrupt init SBpro2 stereo *****************************/

int SBpro2stereo_ok = FALSE;

void interrupt intr_handler_SBpro2stereo()
{
    inp(_sb_card->pollport);

    clear_intr(_sb_card->irq);

    SBpro2stereo_ok = TRUE;
}

/******************************************************************************
******************************************************************************/

#define lo(value) (unsigned char)( value & 0x00FF )
#define hi(value) (unsigned char)((value & 0xFF00) >> 8)

#define DSP_CMD_SPEAKER_ON      0xD1
#define DSP_CMD_SPEAKER_OFF     0xD3
#define DSP_GET_VERSION         0xE1

#define DSP_CMD_8BIT_OUT         0x14
#define DSP_CMD_8BIT_IN          0x24
#define DSP2_CMD_8BITAUTO_OUT    0x1C
#define DSP2_CMD_8BITAUTO_IN     0x2C
#define DSP2_CMD_8BITAUTOHIGH_OUT 0x90
#define DSP2_CMD_8BITAUTOHIGH_IN  0x98

#define DSP_TIME_CONSTANT        0x40
#define DSP4_OUTPUT_RATE         0x41
#define DSP4_INPUT_RATE          0x42
#define DSP2_BLOCK_SIZE          0x48

#define DSP4_CMD_8BITAUTO_OUT    0xC6
#define DSP4_CMD_8BITAUTO_IN     0xCE
#define DSP4_CMD_16BITAUTO_OUT   0xB6
#define DSP4_CMD_16BITAUTO_IN    0xBE

#define DSP_DMA_START_8BIT       0xC0
#define DSP_DMA_PAUSE_8BIT       0xD0
#define DSP_DMA_CONTINUE_8BIT    0xD4
#define DSP2_DMA_EXIT_AUTO_8BIT  0xDA

#define DSP4_DMA_START_16BIT     0xB0
#define DSP4_DMA_PAUSE_16BIT     0xD5
#define DSP4_DMA_CONTINUE_16BIT  0xD6
#define DSP4_DMA_EXIT_AUTO_16BIT 0xD9

#define DSP4_MODE_MONO_US        0x00
#define DSP4_MODE_MONO_S         0x10
#define DSP4_MODE_STEREO_US      0x20
#define DSP4_MODE_STEREO_S       0x30

void write_dsp(SB_CARD *sb_card, unsigned char value)
{
    int i;

    for (i=0; i<0x1000; i++)
    {
        if ((inp(sb_card->writeport) & 0x80) == 0)
        {
            outp(sb_card->writeport, value);
            break;
        }
    }
}

unsigned char read_dsp(SB_CARD *sb_card)
{
    unsigned char value;
    int i;

    for (i=0; i<0x1000; i++)
    {
        if ((inp(sb_card->pollport) & 0x80) != 0)
        {
            value = inp(sb_card->readport);
            break;
        }
    }

    return value;
}

int reset_dsp(SB_CARD *sb_card)
{
    int retval = TRUE;

    outp(sb_card->resetport, 1);
    delay(1);
    outp(sb_card->resetport, 0);
    if (read_dsp(sb_card) != 0xAA)
        retval = FALSE;

    return retval;
}

unsigned char getTimeConstant()
{
    unsigned short TimeConstant = 65536-(256000000/(_sb_sample->channels*_sb_sample->samplingrate));
    return hi(TimeConstant);
}

/**************
***** SB1 *****
**************/

void start_sb_8b_mono_single()
{
    write_dsp(_sb_card, DSP_TIME_CONSTANT);
    write_dsp(_sb_card, getTimeConstant());

    write_dsp(_sb_card, DSP_CMD_8BIT_OUT);
    write_dsp(_sb_card, lo(_dma_buffer->buf_size-1));
    write_dsp(_sb_card, hi(_dma_buffer->buf_size-1));
}

void stop_sb_8b_mono_single()
{
    write_dsp(_sb_card, DSP_CMD_SPEAKER_OFF);

    reset_dsp(_sb_card);

    clear_intr(_sb_card->irq);

    uninstall_handler(_sb_card->irq);
}

int init_sb_8b_mono_single(SB_CARD *sb_card, SB_SAMPLE *sb_sample, DMA_BUFFER *dma_buffer)
{
    _sb_card = sb_card;
    _sb_sample = sb_sample;
    _dma_buffer = dma_buffer;

    updateDmaBuffer();

    if (!reset_dsp(_sb_card))
        return FALSE;

    write_dsp(_sb_card, DSP_CMD_SPEAKER_OFF);
    write_dsp(_sb_card, DSP_CMD_SPEAKER_ON);

    install_handler(_sb_card->irq, intr_handler_single);
    atexit(stop_sb_8b_mono_single);
    dma_buffer->setDmaTransfer(_dma_buffer, _sb_card->dma8, DmaModeWriteTransferSingle);

    return TRUE;
}

/**************
***** SB2 *****
**************/

void start_sb_8b_mono_auto()
{
    write_dsp(_sb_card, DSP_TIME_CONSTANT);
    write_dsp(_sb_card, getTimeConstant());

    write_dsp(_sb_card, DSP2_BLOCK_SIZE);
    write_dsp(_sb_card, lo(_dma_buffer->buf_size-1));
    write_dsp(_sb_card, hi(_dma_buffer->buf_size-1));

    write_dsp(_sb_card, DSP2_CMD_8BITAUTO_OUT);
}

void start_sb_8b_mono_auto_high()
{
    write_dsp(_sb_card, DSP_TIME_CONSTANT);
    write_dsp(_sb_card, getTimeConstant());

    write_dsp(_sb_card, DSP2_BLOCK_SIZE);
    write_dsp(_sb_card, lo(_dma_buffer->buf_size-1));
    write_dsp(_sb_card, hi(_dma_buffer->buf_size-1));

    write_dsp(_sb_card, DSP2_CMD_8BITAUTOHIGH_OUT);
}

void stop_sb_8b_mono_auto()
{
    write_dsp(_sb_card, DSP2_DMA_EXIT_AUTO_8BIT);

    write_dsp(_sb_card, DSP_CMD_SPEAKER_OFF);

    reset_dsp(_sb_card);

    clear_intr(_sb_card->irq);

    uninstall_handler(_sb_card->irq);
}

int init_sb_8b_mono_auto(SB_CARD *sb_card, SB_SAMPLE *sb_sample, DMA_BUFFER *dma_buffer)
{
    _sb_card = sb_card;
    _sb_sample = sb_sample;
    _dma_buffer = dma_buffer;

    updateDmaBuffer();
    updateDmaBuffer();

    if (!reset_dsp(_sb_card))
        return FALSE;

    write_dsp(_sb_card, DSP_CMD_SPEAKER_OFF);
    write_dsp(_sb_card, DSP_CMD_SPEAKER_ON);

    install_handler(_sb_card->irq, intr_handler_auto);
    atexit(stop_sb_8b_mono_auto);
    dma_buffer->setDmaTransfer(_dma_buffer, _sb_card->dma8, DmaModeWriteTransferAuto);

    return TRUE;
}

/*****************
***** SBPRO2 *****
*****************/

unsigned char bSBpro2Filter;

void start_sb_8b_stereo_auto_high()
{
    write_dsp(_sb_card, DSP_TIME_CONSTANT);
    write_dsp(_sb_card, getTimeConstant());

    if (_sb_sample->channels==2)
    {
        outp(_sb_card->mixeraddr, 0xE);
        bSBpro2Filter = inp(_sb_card->mixerdata);
        outp(_sb_card->mixerdata, (bSBpro2Filter | 0x20));
    }

    write_dsp(_sb_card, DSP2_BLOCK_SIZE);
    write_dsp(_sb_card, lo(_dma_buffer->buf_size-1));
    write_dsp(_sb_card, hi(_dma_buffer->buf_size-1));

    write_dsp(_sb_card, DSP2_CMD_8BITAUTOHIGH_OUT);
}

void stop_sb_8b_stereo_auto_high()
{
    if (_sb_sample->channels==2)
    {
        unsigned char bTmp;

        outp(_sb_card->mixeraddr, 0xE);
        outp(_sb_card->mixerdata, bSBpro2Filter);

        outp(_sb_card->mixeraddr, 0xE);
        bTmp = inp(_sb_card->mixerdata);
        outp(_sb_card->mixerdata, (bTmp & 0xFD));
    }

    write_dsp(_sb_card, DSP_CMD_SPEAKER_OFF);

    reset_dsp(_sb_card);

    clear_intr(_sb_card->irq);

    uninstall_handler(_sb_card->irq);
}

int init_sb_8b_stereo_auto_high(SB_CARD *sb_card, SB_SAMPLE *sb_sample, DMA_BUFFER *dma_buffer)
{
    _sb_card = sb_card;
    _sb_sample = sb_sample;
    _dma_buffer = dma_buffer;

    updateDmaBuffer();
    updateDmaBuffer();

    if (!reset_dsp(_sb_card))
        return FALSE;

    write_dsp(_sb_card, DSP_CMD_SPEAKER_OFF);
    write_dsp(_sb_card, DSP_CMD_SPEAKER_ON);

    if (_sb_sample->channels==2)
    {
        unsigned char bTmp;
        SBpro2stereo_ok = FALSE;

        install_handler(_sb_card->irq, intr_handler_SBpro2stereo);

        outp(_sb_card->mixeraddr, 0xE);
        bTmp = inp(_sb_card->mixerdata);
        outp(_sb_card->mixerdata, (bTmp | 0x2));

        dma_buffer->setDmaTransferLength(_dma_buffer, _sb_card->dma8, DmaModeWriteTransferSingle, 2);

        write_dsp(_sb_card, DSP_CMD_8BIT_OUT);
        write_dsp(_sb_card, 0);
        write_dsp(_sb_card, 0);

        while (SBpro2stereo_ok == FALSE)
        {
            ;
        }
        uninstall_handler(_sb_card->irq);
    }

    install_handler(_sb_card->irq, intr_handler_auto);
    atexit(stop_sb_8b_stereo_auto_high);
    dma_buffer->setDmaTransfer(_dma_buffer, _sb_card->dma8, DmaModeWriteTransferAuto);

    return TRUE;
}

/***************
***** SB16 *****
***************/

void start_sb16()
{
    write_dsp(_sb_card, DSP4_OUTPUT_RATE);
    write_dsp(_sb_card, hi(_sb_sample->samplingrate));
    write_dsp(_sb_card, lo(_sb_sample->samplingrate));

    if (_sb_sample->bps==1)
    {
        write_dsp(_sb_card, DSP4_CMD_8BITAUTO_OUT);
        write_dsp(_sb_card, (_sb_sample->channels==2) ? DSP4_MODE_STEREO_US : DSP4_MODE_MONO_US);
    }
    else
    {
        write_dsp(_sb_card, DSP4_CMD_16BITAUTO_OUT);
        write_dsp(_sb_card, (_sb_sample->channels==2) ? DSP4_MODE_STEREO_S : DSP4_MODE_MONO_S);
    }

    write_dsp(_sb_card, lo(_dma_buffer->sample_size-1));
    write_dsp(_sb_card, hi(_dma_buffer->sample_size-1));
}

void stop_sb16()
{
    if (_sb_sample->bps==1)
        write_dsp(_sb_card, DSP2_DMA_EXIT_AUTO_8BIT);
    else
        write_dsp(_sb_card, DSP4_DMA_EXIT_AUTO_16BIT);

    reset_dsp(_sb_card);

    clear_intr(_sb_card->irq);

    uninstall_handler(_sb_card->irq);
}

int init_sb16(SB_CARD *sb_card, SB_SAMPLE *sb_sample, DMA_BUFFER *dma_buffer)
{
    _sb_card = sb_card;
    _sb_sample = sb_sample;
    _dma_buffer = dma_buffer;

    disable();
    updateDmaBuffer();
    updateDmaBuffer();

    if (!reset_dsp(_sb_card))
        return FALSE;

    write_dsp(_sb_card, DSP_CMD_SPEAKER_OFF);
    write_dsp(_sb_card, DSP_CMD_SPEAKER_ON);

    install_handler(_sb_card->irq, intr_handler_auto);
    atexit(stop_sb16);
    dma_buffer->setDmaTransfer(_dma_buffer, (_sb_sample->bps==2) ? _sb_card->dma16:_sb_card->dma8, DmaModeWriteTransferAuto);
    enable();

    return TRUE;
}

/*****************
***** SBFAKE *****
*****************/

int init_fake(SB_CARD *sb_card, SB_SAMPLE *sb_sample, DMA_BUFFER *dma_buffer)
{
    return FALSE;
}

void stop_fake()
{
}

void start_fake()
{
}

/*****************************************************************************/

int detect_sb_configuration(SB_CARD *sb_card)
{
    if (sb_card != NULL)
    {
        char *blaster = getenv("BLASTER");

        sb_card->baseio = 0x220;
        sb_card->irq = 5;
        sb_card->dma8 = 1;
        sb_card->dma16 = 0;

        if (blaster != NULL)
        {
            while (*blaster != NULL)
            {
                /* skiping spaces */
                while ((*blaster == ' ') || (*blaster == '\t'))
                    blaster++;

                if (*blaster != NULL)
                {
                    switch (*blaster)
                    {
                    case 'A':
                    case 'a':
                        sb_card->baseio = strtol(blaster+1, NULL, 16);
                        break;
                    case 'I':
                    case 'i':
                        sb_card->irq = strtol(blaster+1, NULL, 10);
                        break;
                    case 'D':
                    case 'd':
                        sb_card->dma8 = strtol(blaster+1, NULL, 10);
                        break;
                    case 'H':
                    case 'h':
                        sb_card->dma16 = strtol(blaster+1, NULL, 10);
                        break;
                    }
                }

                /* going to next space */
                while ((*blaster != NULL) && (*blaster != ' ') && (*blaster != '\t'))
                    blaster++;
            }

            printf("SB card configuration : A=%x I=%d D=%d H=%d\n", sb_card->baseio, sb_card->irq, sb_card->dma8, sb_card->dma16);

            return TRUE;
        }
    }
    return FALSE;
}

SB_CARD *createSBCard(int forcedspversion)
{
    SB_CARD *sb_card = malloc(sizeof(SB_CARD));
    if (sb_card != NULL)
    {
        sb_card->init = init_fake;
        sb_card->play = start_fake;
        sb_card->stop = stop_fake;
        sb_card->support16bits = FALSE;
        sb_card->supportStereo = FALSE;
        sb_card->supportMinMono = 0;
        sb_card->supportMaxMono = 0;
        sb_card->supportMinStereo = 0;
        sb_card->supportMaxStereo = 0;

        if (detect_sb_configuration(sb_card))
        {
            sb_card->mixeraddr  = sb_card->baseio + 0x04;
            sb_card->mixerdata  = sb_card->baseio + 0x05;
            sb_card->resetport  = sb_card->baseio + 0x06;
            sb_card->readport   = sb_card->baseio + 0x0A;
            sb_card->writeport  = sb_card->baseio + 0x0C;
            sb_card->pollport   = sb_card->baseio + 0x0E;
            sb_card->poll16port = sb_card->baseio + 0x0F;

            if (reset_dsp(sb_card))
            {
                if (forcedspversion != 0)
                {
                    sb_card->dspversion = forcedspversion;
                }
                else
                {
                    unsigned char cMajorVersion, cMinorVersion;
                    write_dsp(sb_card, DSP_GET_VERSION);
                    cMajorVersion = read_dsp(sb_card);
                    cMinorVersion = read_dsp(sb_card);
                    sb_card->dspversion = cMajorVersion;
                    sb_card->dspversion <<= 8;
                    sb_card->dspversion += cMinorVersion;
                }
                printf("SB card dsp version : 0x%x\n", sb_card->dspversion);

                sb_card->supportDmaAuto = TRUE;

                if (sb_card->dspversion >= 0x400)
                {
                    /* SoundBlaster 16, CT-17xx, CT-22xx, CT-27xx, CT-28xx */
                    sb_card->init = init_sb16;
                    sb_card->play = start_sb16;
                    sb_card->stop = stop_sb16;
                    sb_card->support16bits = TRUE;
                    sb_card->supportStereo = TRUE;
                    sb_card->supportMinMono = 5000;
                    sb_card->supportMaxMono = 44100;
                    sb_card->supportMinStereo = 5000;
                    sb_card->supportMaxStereo = 44100;
                }
                else if (sb_card->dspversion >= 0x300)
                {
                    /* 8 bit stereo auto-initialize high-speed
                       SoundBlaster Pro, CT-1330
                       SoundBlaster Pro 2, CT-16x0
                    */
                    sb_card->init = init_sb_8b_stereo_auto_high;
                    sb_card->play = start_sb_8b_stereo_auto_high;
                    sb_card->stop = stop_sb_8b_stereo_auto_high;
                    sb_card->support16bits = FALSE;
                    sb_card->supportStereo = TRUE;
                    sb_card->supportMinMono = 4000;
                    sb_card->supportMaxMono = 44100;
                    sb_card->supportMinStereo = 11025;
                    sb_card->supportMaxStereo = 22050;
                }
                else if (sb_card->dspversion >= 0x201)
                {
                    /* 8 bit mono auto-initialize high-speed
                       SoundBlaster 2.0, CT-1350
                    */
                    sb_card->init = init_sb_8b_mono_auto;
                    sb_card->play = start_sb_8b_mono_auto_high;
                    sb_card->stop = stop_sb_8b_mono_auto;
                    sb_card->support16bits = FALSE;
                    sb_card->supportStereo = FALSE;
                    sb_card->supportMinMono = 4000;
                    sb_card->supportMaxMono = 44100;
                    sb_card->supportMinStereo = 0;
                    sb_card->supportMaxStereo = 0;
                }
                else if (sb_card->dspversion == 0x200)
                {
                    /* 8 bit mono auto-initialize
                       SoundBlaster 1.5, CT-1320C
                    */
                    sb_card->init = init_sb_8b_mono_auto;
                    sb_card->play = start_sb_8b_mono_auto;
                    sb_card->stop = stop_sb_8b_mono_auto;
                    sb_card->support16bits = FALSE;
                    sb_card->supportStereo = FALSE;
                    sb_card->supportMinMono = 4000;
                    sb_card->supportMaxMono = 23000;
                    sb_card->supportMinStereo = 0;
                    sb_card->supportMaxStereo = 0;
                }
                else if (sb_card->dspversion > 0x100)
                {
                    /* 8 bit mono single-cycle
                       SoundBlaster 1.0, CT-1320A (DSP 1.05)
                    */
                    sb_card->init = init_sb_8b_mono_single;
                    sb_card->play = start_sb_8b_mono_single;
                    sb_card->stop = stop_sb_8b_mono_single;
                    sb_card->support16bits = FALSE;
                    sb_card->supportStereo = FALSE;
                    sb_card->supportDmaAuto = FALSE;
                    sb_card->supportMinMono = 4000;
                    sb_card->supportMaxMono = 23000;
                    sb_card->supportMinStereo = 0;
                    sb_card->supportMaxStereo = 0;
                }
            }
        }
    }

    return sb_card;
}

int testCardCapabilities(SB_CARD *sb_card, SB_SAMPLE *sb_sample)
{
    int retval = TRUE;

    if ((sb_sample->bps == 2) && (sb_card->support16bits == FALSE))
        retval = FALSE;
    else
    {
        switch (sb_sample->channels)
        {
        case 2:
            if (sb_card->supportStereo == FALSE)
                retval = FALSE;
            else
            {
                if ((sb_sample->samplingrate<sb_card->supportMinStereo) || (sb_sample->samplingrate>sb_card->supportMaxStereo))
                    retval = FALSE;
            }
            break;

        case 1:
            if ((sb_sample->samplingrate<sb_card->supportMinMono) || (sb_sample->samplingrate>sb_card->supportMaxMono))
                retval = FALSE;
            break;

        default:
            retval = FALSE;
            break;
        }
    }

    return retval;
}
