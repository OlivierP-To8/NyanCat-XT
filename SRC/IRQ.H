/*******************************************************************************
* Author  : OlivierP                                                           *
* Date    : october & november 2012                                            *
* License : GNU GPLv3 (http://www.gnu.org/copyleft/gpl.html)                   *
*******************************************************************************/

#ifndef IRQ_H
#define IRQ_H

#include <dos.h>

#define TRUE  1
#define FALSE 0

typedef void (interrupt *INTERRUPT_HANDLER)(void);

void clear_intr(char irq);
void install_handler(char irq, INTERRUPT_HANDLER ih);
void uninstall_handler(char irq);

#endif
