/*******************************************************************************
* Author  : OlivierP                                                           *
* Date    : october & november 2012                                            *
* License : GNU GPLv3 (http://www.gnu.org/copyleft/gpl.html)                   *
*******************************************************************************/

#ifndef RAM_H
#define RAM_H

#include <mem.h>

#define TRUE  1
#define FALSE 0

typedef struct
{
    unsigned short xms_handle;
    unsigned char far *buf_ptr;
    unsigned long buf_size;
} RAM_BUFFER;

RAM_BUFFER *createRamBuffer(unsigned long size);
void copyFromRamBuffer(RAM_BUFFER *buf_ram, unsigned long pos, unsigned long size, unsigned char far *dst_ptr);
void copyToRamBuffer(RAM_BUFFER *buf_ram, unsigned long pos, unsigned long size, unsigned char far *src_ptr);
void freeRamBuffer(RAM_BUFFER *buf_ram);

#endif
