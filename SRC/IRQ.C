/*******************************************************************************
* Author  : OlivierP                                                           *
* Date    : october & november 2012                                            *
* License : GNU GPLv3 (http://www.gnu.org/copyleft/gpl.html)                   *
*******************************************************************************/

#include <stdio.h>
#include <conio.h>
#include "irq.h"

#ifdef __WATCOMC__
#include <malloc.h>
#define outportb outp
#define inportb inp
#define disable _disable
#define enable _enable
#define getvect _dos_getvect
#define setvect _dos_setvect
#endif

#define MASTER_PIC_CMD 0x20
#define MASTER_PIC_DAT 0x21
#define SLAVE_PIC_CMD  0xA0
#define SLAVE_PIC_DAT  0xA1
#define PIC_EOI        0x20

INTERRUPT_HANDLER oldintvector = NULL;
int handlerinstalled;

void clear_intr(char irq)
{
    if (irq > 7)
    {
        outportb(SLAVE_PIC_CMD, PIC_EOI);
    }
    outportb(MASTER_PIC_CMD, PIC_EOI);
}

void enable_intr(char irq)
{
    if (irq > 7)
    {
        outportb(MASTER_PIC_DAT, inp(MASTER_PIC_DAT) & ~(1 << 2));
        outportb(SLAVE_PIC_DAT,  inp(SLAVE_PIC_DAT)  & ~(1 << (irq & 7)));
    }
    else
    {
        outportb(MASTER_PIC_DAT, inp(MASTER_PIC_DAT) & ~(1 << (irq & 7)));
    }
}

void disable_intr(char irq)
{
    if (irq > 7)
    {
        outportb(SLAVE_PIC_DAT,  inp(SLAVE_PIC_DAT)  | (1 << (irq & 7)));
    }
    else
    {
        outportb(MASTER_PIC_DAT, inp(MASTER_PIC_DAT) | (1 << (irq & 7)));
    }
}

void install_handler(char irq, INTERRUPT_HANDLER ih)
{
    char irq_intvector;

    disable();
    disable_intr(irq);

    if (irq > 7)
    {
        irq_intvector = 0x70 + (irq & 7);
    }
    else
    {
        irq_intvector = 0x08 + (irq & 7);
    }

    oldintvector = getvect(irq_intvector);
    setvect(irq_intvector, ih);

    enable_intr(irq);
    enable();

    handlerinstalled = TRUE;
}

void uninstall_handler(char irq)
{
    char irq_intvector;

    if (handlerinstalled == TRUE)
    {
        disable();
        disable_intr(irq);

        if (irq > 7)
        {
            irq_intvector = 0x70 + (irq & 7);
        }
        else
        {
            irq_intvector = 0x08 + (irq & 7);
        }

        setvect(irq_intvector, oldintvector);

        enable();

        handlerinstalled = FALSE;
    }
}
