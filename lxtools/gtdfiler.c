/* --------------------------------------------------------------------
   Project: HP200LX FILER PROTOCOL (CLIENT) COMMUNICATIONS FOR PAL
   Module:  GTDFILER.C
   Author:  Harry Konstas / Gilles Kohl
   Started: 17. Oct. 95
   Subject: Get directory entry from Server
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
		  Get directory entry from server
   -------------------------------------------------------------------- */

int  FilerGetDir(FILERCOM *pFiler)
{
   int f, stat;
   unsigned long ul;

   /* send directory name */
   stat = FilerRequest(pFiler, GET_DIR, 0, NULL);
   if (stat == CANNOT_GET_ENTRY)
      return stat;
   
   /* re-synchronize */
   if(stat != SERVER_ACK) {
      FilerSync(pFiler);
      return CANNOT_GET_ENTRY;
   }

   /* retrieve filename */
   for(f=0;f<14;f++)
      pFiler->Name[f]=pFiler->pData[f+9];

   /* retrieve date information */
   ul = (pFiler->pData[1]      ) |
        (pFiler->pData[2] <<  8) |
        (pFiler->pData[3] << 16) |
        (pFiler->pData[4] << 24);
   pFiler->DateStamp.year =  ul >> 25;
   pFiler->DateStamp.month = ul >> 21;
   pFiler->DateStamp.day =   ul >> 16;
   pFiler->DateStamp.hour =  ul >> 11;
   pFiler->DateStamp.min =   ul >> 5;
   pFiler->DateStamp.sec =   ul;

   /* retrieve file size */
   pFiler->FileSize = (pFiler->pData[5]      ) |
		      (pFiler->pData[6] <<  8) |
		      (pFiler->pData[7] << 16) |
		      (pFiler->pData[8] << 24);

   /* retrieve attribute */
   pFiler->Attribute = pFiler->pData[0];

   return GOT_DIR_ENTRY;

}

