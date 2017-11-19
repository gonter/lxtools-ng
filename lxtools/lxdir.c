/* --------------------------------------------------------------------
   Project: Communication package Linux-HPx00LX Filer
   Module:  lxdir.c
   Author:  A. Garzotto
   Started: 28. Nov. 95
   Subject: Display directory contents
   -------------------------------------------------------------------- */

/* --------------------------------------------------------------------
                             includes
   -------------------------------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>

/* --------------------------------------------------------------------
                         local includes
   -------------------------------------------------------------------- */

#include "pal.h"
#include "palpriv.h"
#include "config.h"


/* --------------------------------------------------------------------
                        display help
   -------------------------------------------------------------------- */

static void help(void)
{
   fprintf(USE_OUT, "USAGE: lxdir [options] <directory>\n");
   fprintf(USE_OUT, "  options: -<n> sets comm port <n>\n");
   fprintf(USE_OUT, "           -b <baud> sets baud rate to <baud>\n");
   exit(EXITVAL(1));
}

/* --------------------------------------------------------------------
                        make DOS directory string
   -------------------------------------------------------------------- */

static void makedir(char *dir)
{
   char *p = dir;
   int hasdot = 0;
   int hasstar = 0;
   
   while (*p)
   {
      if (*p == '/') *p = '\\';
      if (*p == '.') hasdot = 1;
      if (*p == '*') hasstar = 1;
      p++;
   }
   if (!hasdot && !hasstar)
   {
      if (p[-1] == '\\')
	 strcat(dir, "*.*");
      else
	 strcat(dir, "\\*.*");
   }
}

/* --------------------------------------------------------------------
                               M A I N
   -------------------------------------------------------------------- */

int main (int argc, char **argv)
{
   int   stat;
   int port = 1;
   int speed = DEF_BAUD;
   FILERCOM *pFiler;
   char dir[256] = "";
   int i = 1;

   fprintf(USE_OUT, "LXDIR %s by A. Garzotto\n\n", VERSION);

   while (i < argc)
   {
      if (argv[i][0] == '-')
      {
	 switch (argv[i][1])
	 {
	  case '1':
	  case '2':
	  case '3':
	  case '4':
	  case '5':
	  case '6':
	  case '7':
	  case '8': port = argv[i][1] - '0'; break;
	  case 'B':
	  case 'b': speed = atoi(argv[++i]); break;
	  default: help(); break;
	 }
      }
      else
	 strcpy(dir, argv[i]);
      i++;      
   }
   if (!*dir) help();
   makedir(dir);
   
   if(!(pFiler = FilerConnect(port, speed, &FlCb))) {
      fprintf(USE_OUT, "\nUnable to connect to palmtop!\n");
      exit(EXITVAL(3));
   }

   stat = FilerAskDir(pFiler, dir);
   if (stat == NO_RESPONSE)
       fprintf(USE_OUT, "\nServer Not responding.\n");
   else if (stat == BAD_REQUEST)
       fprintf(USE_OUT, "\nBad request.\n");
   else if (stat != SERVER_ACK)
       fprintf(USE_OUT, "\nTransmission error.\n");

   fprintf(USE_OUT, " Directory of %s:\n\n", dir);
   while (1)
   {
      if(FilerGetDir(pFiler) == CANNOT_GET_ENTRY) break;
      if (pFiler->Attribute & 0x10)
         fprintf(USE_OUT, "%-12s    <DIR>      %02d-%02d-%02d  %02d:%02d\n",
               pFiler->Name,
               pFiler->DateStamp.month, pFiler->DateStamp.day,
               (pFiler->DateStamp.year+80) % 100,
               pFiler->DateStamp.hour, pFiler->DateStamp.min);
      else
         fprintf(USE_OUT, "%-12s %12lu  %02d-%02d-%02d  %02d:%02d\n",
               pFiler->Name, pFiler->FileSize,
               pFiler->DateStamp.month, pFiler->DateStamp.day,
               (pFiler->DateStamp.year+80) % 100,
               pFiler->DateStamp.hour, pFiler->DateStamp.min);
   }
   
   FilerDisconnect(pFiler);
   exit(EXITVAL(0));
}

