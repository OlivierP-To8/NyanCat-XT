/*******************************************************************************
* Author  : OlivierP                                                           *
* Date    : october & november 2012                                            *
* License : GNU GPLv3 (http://www.gnu.org/copyleft/gpl.html)                   *
*******************************************************************************/

#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wav.h"
#include "ram.h"

typedef struct
{
    unsigned long ID;
    unsigned long Size;
} CHUNK;

SB_SAMPLE *PreLoadWAV(const char *FileName)
{
    SB_SAMPLE *retval = NULL;
    CHUNK Chunk;
    FILE *WAVFile = fopen(FileName, "rb");

    if (FileName == NULL)
        return NULL;

    if (WAVFile == NULL)
    {
        printf("Can't open %s\n", FileName);
        return NULL;
    }
    /* else
    {
        printf("Loading %s\n", FileName);
    } */

    fread(&Chunk, sizeof(CHUNK), 1, WAVFile);
    if (Chunk.ID == 0x46464952) /* RIFF */
    {
        unsigned long Type;
        fread(&Type, sizeof(Type), 1, WAVFile);
        if (Type == 0x45564157) /* WAVE */
        {
            fread(&Chunk, sizeof(CHUNK), 1, WAVFile);
            if (Chunk.ID == 0x20746D66) /* fmt */
            {
                unsigned short CompressionCode, NbChannels, BitPerSample, BlockAlign;
                unsigned long SampleRate, AverageBPS;

                fread(&CompressionCode, sizeof(CompressionCode), 1, WAVFile);
                fread(&NbChannels, sizeof(NbChannels), 1, WAVFile);
                fread(&SampleRate, sizeof(SampleRate), 1, WAVFile);
                fread(&AverageBPS, sizeof(AverageBPS), 1, WAVFile);
                fread(&BlockAlign, sizeof(BlockAlign), 1, WAVFile);
                fread(&BitPerSample, sizeof(BitPerSample), 1, WAVFile);

                /*
                printf("Compression : %d\n", CompressionCode);
                printf("Channels : %d\n", NbChannels);
                printf("Frequency : %u Hz\n", SampleRate);
                printf("BitRes : %d\n", BitPerSample);
                */

                if ((CompressionCode == 1) && (Chunk.Size == 16))
                {
                    fread(&Chunk, sizeof(CHUNK), 1, WAVFile);
                    if (Chunk.ID == 0x61746164) /* data */
                    {
                        unsigned long size = Chunk.Size;
                        retval = malloc(sizeof(SB_SAMPLE));
                        if (retval == NULL)
                            printf("Not enough memory\n");
                        else
                        {
                            retval->sample = NULL;
                            retval->samplingrate = SampleRate;
                            retval->channels = NbChannels;
                            retval->bps = BitPerSample/8;
                            retval->done = FALSE;
                            retval->loop = FALSE;
                            retval->curpos = 0;
                        }
                    }
                    else
                        printf("bad structure\n");
                }
                else
                    printf("WAV file must be uncompressed\n");
            }
        }
    }

    fclose(WAVFile);

    return retval;
}

SB_SAMPLE *LoadWAV(const char *FileName, unsigned long bufsize, unsigned char far *bufptr)
{
    SB_SAMPLE *retval = NULL;
    CHUNK Chunk;
    FILE *WAVFile = fopen(FileName, "rb");

    if (FileName == NULL)
        return NULL;

    if (WAVFile == NULL)
    {
        printf("Can't open %s\n", FileName);
        return NULL;
    }
    /* else
    {
        printf("Loading %s\n", FileName);
    } */

    fread(&Chunk, sizeof(CHUNK), 1, WAVFile);
    if (Chunk.ID == 0x46464952) /* RIFF */
    {
        unsigned long Type;
        fread(&Type, sizeof(Type), 1, WAVFile);
        if (Type == 0x45564157) /* WAVE */
        {
            fread(&Chunk, sizeof(CHUNK), 1, WAVFile);
            if (Chunk.ID == 0x20746D66) /* fmt */
            {
                unsigned short CompressionCode, NbChannels, BitPerSample, BlockAlign;
                unsigned long SampleRate, AverageBPS;

                fread(&CompressionCode, sizeof(CompressionCode), 1, WAVFile);
                fread(&NbChannels, sizeof(NbChannels), 1, WAVFile);
                fread(&SampleRate, sizeof(SampleRate), 1, WAVFile);
                fread(&AverageBPS, sizeof(AverageBPS), 1, WAVFile);
                fread(&BlockAlign, sizeof(BlockAlign), 1, WAVFile);
                fread(&BitPerSample, sizeof(BitPerSample), 1, WAVFile);

                /*
                printf("Compression : %d\n", CompressionCode);
                printf("Channels : %d\n", NbChannels);
                printf("Frequency : %u Hz\n", SampleRate);
                printf("BitRes : %d\n", BitPerSample);
                */

                if ((CompressionCode == 1) && (Chunk.Size == 16))
                {
                    fread(&Chunk, sizeof(CHUNK), 1, WAVFile);
                    if (Chunk.ID == 0x61746164) /* data */
                    {
                        unsigned long size = Chunk.Size;
                        RAM_BUFFER *buf_ram = createRamBuffer(size);

                        if (buf_ram == NULL)
                            printf("Not enough memory for a %lu bytes sample\n", size);
                        else
                        {
                            retval = malloc(sizeof(SB_SAMPLE));
                            if (retval == NULL)
                                printf("Not enough memory\n");
                            else
                            {
                                unsigned long total = 0;

                                retval->sample = buf_ram;
                                retval->samplingrate = SampleRate;
                                retval->channels = NbChannels;
                                retval->bps = BitPerSample/8;
                                retval->done = FALSE;
                                retval->loop = FALSE;
                                retval->curpos = 0;

                                while (total < size)
                                {
                                    unsigned long t = (size-total) > bufsize ? bufsize : size-total;
                                    unsigned long nb = fread(bufptr, 1, t, WAVFile);
                                    copyToRamBuffer(retval->sample, total, nb, bufptr);
                                    total += nb;
                                }
                                retval->sample->buf_size = total;
                            }
                        }
                    }
                    else
                        printf("bad structure\n");
                }
                else
                    printf("WAV file must be uncompressed\n");
            }
        }
    }

    fclose(WAVFile);

    return retval;
}

