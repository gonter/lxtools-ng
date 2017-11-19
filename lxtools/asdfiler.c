/* --------------------------------------------------------------------
   Project: HP200LX FILER PROTOCOL (CLIENT) COMMUNICATIONS FOR PAL
   Module:  ASDFILER.C
   Author:  Harry Konstas
   Started: 17. Oct. 95
   Subject: Request remote directory from Server
   -------------------------------------------------------------------- */

/* --------------------------------------------------------------------
                       standard includes
   -------------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* --------------------------------------------------------------------
                         local includes
   -------------------------------------------------------------------- */
#include "pal.h"
#include "palpriv.h"

/* --------------------------------------------------------------------
                Request Remote directory from server
   -------------------------------------------------------------------- */

int  FilerAskDir(FILERCOM *pFiler, char *RemoteDir)
{
   /* send directory name */
   return FilerRequest(pFiler, ASK_DIR, strlen(RemoteDir),
                 (BYTE *)RemoteDir);
}

