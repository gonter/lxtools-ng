/* --------------------------------------------------------------------
   Project: PAL: Palmtop Application Library
   Module:  PAL.H
   Author:  Harry Konstas/Gilles Kohl/Andreas Garzotto
   Started: 10. Nov. 94
   Subject: Main PAL include file
   -------------------------------------------------------------------- */

#ifndef _PAL_H
#define _PAL_H

#include <stddef.h>
#include <time.h>
#include <string.h>

/* --------------------------------------------------------------------
                      constant definitions
   -------------------------------------------------------------------- */

#ifndef FALSE
#define FALSE 0
#define TRUE 1
#endif

/* --------------------------------------------------------------------
                           type definitions
   -------------------------------------------------------------------- */

/* some utility types */
typedef unsigned char BYTE;
#ifndef NOWDW
typedef unsigned short int WORD;
typedef unsigned long DWORD;
#endif


/* --------------------------------------------------------------------
                              prototypes
   -------------------------------------------------------------------- */


#include "palfiler.h"

#endif

