#ifndef TURBOC_COMPAT_CONIO_H
#define TURBOC_COMPAT_CONIO_H

#include_next <conio.h>
#include <stdlib.h>

static void turbo_compat_clrscr(void)
{
    (void)system("cls");
}

#define clrscr turbo_compat_clrscr

#endif
