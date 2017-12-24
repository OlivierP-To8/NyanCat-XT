/*******************************************************************************
* Author  : OlivierP                                                           *
* Date    : october & november 2012                                            *
* License : GNU GPLv3 (http://www.gnu.org/copyleft/gpl.html)                   *
*******************************************************************************/

#ifndef WAV_H
#define WAV_H

#include "sndblast.h"

SB_SAMPLE *PreLoadWAV(const char *FileName);
SB_SAMPLE *LoadWAV(const char *FileName, unsigned long bufsize, unsigned char far *bufptr);

#endif
