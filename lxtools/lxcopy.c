/* --------------------------------------------------------------------
   Project: Communication package Linux-HPx00LX Filer
   Module:  lxdir.c
   Author:  A. Garzotto
   Started: 28. Nov. 95
   Last Update: 12-DEC-95
   Subject: Copy between Linux and the palmtop
   -------------------------------------------------------------------- */

/* --------------------------------------------------------------------
                             includes
   -------------------------------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ctype.h>
#include <dirent.h>

#define NIL 0

/* --------------------------------------------------------------------
                         local includes
   -------------------------------------------------------------------- */

#include "pal.h"
#include "palpriv.h"
#include "config.h"

size_t  MySendBlock(void *Buf, size_t Size, void *Handle);
size_t  MyRecvBlock(void *Buf, size_t Size, void *Handle);

/* --------------------------------------------------------------------
                        Global types and vars
   -------------------------------------------------------------------- */

struct list_desc
{
   char *name;
   char *fname;
   int processed;
   struct list_desc *next;
};

typedef struct list_desc *LIST;

LIST inlist = NIL;
LIST outlist = NIL;

int recursive = 0;
int archive = 0;
int overwrite = 1;

/* --------------------------------------------------------------------
                        display help
   -------------------------------------------------------------------- */

static void help(void)
{
   fprintf(USE_OUT, "USAGE: lxcopy [options] <source file > {<source file>} <dest file>\n");
   fprintf(USE_OUT, "  options: -<n> sets comm port <n>\n");
   fprintf(USE_OUT, "           -b <baud> sets baud rate to <baud>\n");
   fprintf(USE_OUT, "           -r enters subdirectories recursively\n");
   fprintf(USE_OUT, "           -a only copies files with archive bit set\n");
   fprintf(USE_OUT, "           -o do not overwrite existing files\n");
   fprintf(USE_OUT, "  note that the palmtop file name must have a drive specified\n");
   exit(EXITVAL(1));
}

/* --------------------------------------------------------------------
                        make DOS directory string
   -------------------------------------------------------------------- */

static void makedir(char *dir, int stars)
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
   if (stars && !hasdot && !hasstar)
   {
      if (p[-1] == '\\')
	 strcat(dir, "*.*");
      else
	 strcat(dir, "\\*.*");
   }
}

/* --------------------------------------------------------------------
                        extract the directory from a file name
   -------------------------------------------------------------------- */

static void makebase(char *dir, char *base)
{
   char *p;
   
   strcpy(base, dir);
   p = &base[strlen(base) - 1];
   while ((p > base) && (*p != '\\')) p--;
   *p = '\0';
}


/* --------------------------------------------------------------------
                        User defined GET/SEND blocks
   -------------------------------------------------------------------- */

FLCB MyFlCb;

/* User defined Send-block routine, replaces default FlCb */

size_t  MySendBlock(void *Buf, size_t Size, void *Handle)
{
   fprintf(USE_OUT, "."); 
   return fread(Buf, 1, Size, Handle);
}

/* User defined Get-block routine, replaces default FlCb */

size_t  MyRecvBlock(void *Buf, size_t Size, void *Handle)
{
   fprintf(USE_OUT, ".");
   return fwrite(Buf, 1, Size, Handle);
}


/* --------------------------------------------------------------------
                               Add name to file list
   -------------------------------------------------------------------- */

LIST add_list(char *fname, int input, int processed)
{
   LIST li = inlist;
   LIST l = (LIST)malloc(sizeof(struct list_desc));
   
   while (li && (li->next)) li = li->next;
   l->name = (char *)malloc(strlen(fname) + 15);
   strcpy(l->name, fname);
   l->processed = processed;
   l->fname = NIL;
   l->next = NIL;
   if (input)
   {
      if (li)
	 li->next = l;
      else
	 inlist = l;
   }
   else
      outlist = l;
   return l;
}

/* --------------------------------------------------------------------
                        Create Linux dir if it does not exist
   -------------------------------------------------------------------- */

void make_dir(char *path)
{
   struct stat statbuf;
   char *p = path;
   
   while (*p)
   {
      while (*p && (*p != '/')) p++;
      if (*p)
      {
	 *p = '\0';
         if (stat(path, &statbuf))
	 {
	    fprintf(USE_OUT, "Creating '%s'...\n", path);
	    if (mkdir(path, 0777))
	       perror("Could not create directory");
	 }
	 *p = '/';
      }
      p++;
   }
}

/* --------------------------------------------------------------------
                               Copy from LX to Linux
   -------------------------------------------------------------------- */

void from_lx(FILERCOM *pFiler)
{
   int sta, num, is_dir;
   LIST l = inlist;
   LIST l1, l2;
   char *p, source[256], target[256], base[256];
   struct stat statbuf;

   if (recursive)
      fprintf(USE_OUT, "Collecting file names...\n");
   while (l)
   {
      if (!l->processed)
      {
	 makedir(l->name, 1);
	 makebase(l->name, base);
         fprintf(USE_OUT, " %s\n", l->name);
	 sta = FilerAskDir(pFiler, l->name);
         if (sta == NO_RESPONSE) fprintf(USE_OUT, "\nServer Not responding.\n");
	 num = 0;
         while (1)
	 {
            if(FilerGetDir(pFiler) == CANNOT_GET_ENTRY) break;
            if (pFiler->Attribute & 0x10)
	    {
	       if (recursive && (*pFiler->Name != '.'))
	       {
		  sprintf(source, "%s\\%s", base, pFiler->Name);
/*                  fprintf(USE_OUT, "%s\n", source); */
		  l2 = add_list(source , 1, 0);
		  l2->fname = (char *) malloc(128);
		  *l2->fname = '\0';
		  if (l->fname)
		  {
		     strcat(l2->fname, l->fname);
		     strcat(l2->fname, "/");
		  }
		  strcat(l2->fname, (char *)pFiler->Name);
	       }
	       continue;
	    }
	    
	    if (!archive || (pFiler->Attribute & 32))
	    {
	       num++;
	       l1 = add_list(l->name, 1, 1);
	       if (l->fname)
	       {
	          l1->fname = (char *)malloc(strlen(l->fname) + 20);
	          strcpy(l1->fname, l->fname);
	          strcat(l1->fname, "/");
	          strcat(l1->fname, (char *)pFiler->Name);
	       }
	       else
	       {
   	          l1->fname = (char *)malloc(strlen((char *)pFiler->Name) + 1);
	          strcpy(l1->fname, (char *)pFiler->Name);
	       }
               p = &(l1->name[strlen(l1->name) - 1]);
               while (*p != '\\') p--;
               *p = '\0';
/*	       fprintf(USE_OUT, "%s\n", l1->fname); */
	    }
	 }
	 if (!num && !recursive)
	    fprintf(USE_OUT, "No match for '%s'\n", l->name);
      }
      l = l->next;
   }

   stat(outlist->name, &statbuf);
   is_dir = statbuf.st_mode & S_IFDIR;
   l = inlist;
   while (l)
   {
      if (l->processed)
      {
	 p = &l->fname[strlen(l->fname) - 1];
	 while ((p > l->fname) && (*p != '/') && (*p != '\\')) p--;
         if (p > l->fname) p++;
	 sprintf(source, "%s\\%s", l->name, p);
	 if (is_dir)
	 {
            sprintf(target, "%s/%s", outlist->name, l->fname);
	 }
	 else
            sprintf(target, "%s", outlist->name);
#ifdef LOWERCASE
	 p = target;
	 while (*p) *(p++) = tolower(*p);
#endif
	 make_dir(target);
	 if (overwrite || stat(target, &statbuf))
	 {
	    fprintf(USE_OUT, "Copying '%s' to '%s'.", source, target);
            sta = FilerGetFile(pFiler, source, target);
            if (sta !=  GOT_FILE_OK) fprintf(USE_OUT, "\nCannot get file.\n");
	    fprintf(USE_OUT, "\n");
	 }
	 else
	    fprintf(USE_OUT, "Preserving '%s'.\n", target);
      }
      l = l->next;
   }
}


/* --------------------------------------------------------------------
                          Create a directory on the LX
   -------------------------------------------------------------------- */

void make_lxdir(FILERCOM *pFiler, char *dir)
{
   char *p = dir;
   int stat;
   
   while (*p && (*p != '\\')) p++;
   p++;
   while (*p)
   {
      while (*p && (*p != '\\')) p++;
      if (*p)
      {
	 *p = '\0';
	 fprintf(USE_OUT, "Making sure '%s' exists...\n", dir);
	 stat = FilerMakeDir(pFiler, dir);
         if(stat==CANNOT_CREATE_DIR)
	     FilerSync(pFiler);
	 *p = '\\';
      }
      p++;
   }
}

/* --------------------------------------------------------------------
                               Copy from Linux to LX
   -------------------------------------------------------------------- */

void from_linux(FILERCOM *pFiler)
{
   char target[356];
   struct stat statbuf;
   DIR *dir;
   struct dirent *dire;
   LIST l = inlist, l1;
   char *p;
   int sta, is_dir = 0;
   
   if (recursive)
      fprintf(USE_OUT, "Collecting file names...\n");
   while (l)
   {
      if (!stat(l->name, &statbuf))
      {
	 if (statbuf.st_mode & S_IFDIR)
	 {
	    if (recursive)
	    {
               fprintf(USE_OUT, " %s/*\n", l->name);
	       dir = opendir(l->name);
	       while ((dire = readdir(dir)) != 0)
	       {
		  sprintf(target, "%s/%s", l->name, dire->d_name);
		  stat(target, &statbuf);
		  if (*(dire->d_name) != '.')
		  {
		     if (statbuf.st_mode & S_IFDIR)
		     {
		        l1 = add_list(target, 1, 0);
		     }
		     else
		     {
		        l1 = add_list(target, 1, 1);
		     }
	             l1->fname = (char *)malloc(128);
         	     if (l->fname)
		        strcpy(l1->fname, l->fname);
		     else
		        strcpy(l1->fname, "");
		     strcat(l1->fname, "\\");
		     strcat(l1->fname, dire->d_name);
		  }
	       }
	    }
	 }
	 else
	    l->processed = 1;
      }
      l = l->next;
   }

   makedir(outlist->name, 0);
   sta = FilerAskDir(pFiler, outlist->name);
   if (sta == NO_RESPONSE) fprintf(USE_OUT, "\nServer Not responding.\n");
   while (1)
   {
      if(FilerGetDir(pFiler) == CANNOT_GET_ENTRY) break;
      if (pFiler->Attribute & 0x10) is_dir = 1;
   }
   if (outlist->name[strlen(outlist->name) - 1] == '\\')
   {
      is_dir = 1;
      outlist->name[strlen(outlist->name) - 1] = '\0';
   }
   
   l = inlist;
   while (l)
   {
      if (l->processed)
      {
         strcpy(target, outlist->name);
         if (is_dir)
         {
	    if (recursive)
	    {
	       strcat(target, l->fname);
	    }
	    else
	    {
	       p = &(l->name[strlen(l->name) - 1]);
	       while ((p > l->name) && (*p != '/')) p--;
	       if (*p == '/') p++;
    	       strcat(target, "\\");
	       strcat(target, p);
	    }
         }
         fprintf(USE_OUT, "Copying '%s' to '%s'.", l->name, target);
         sta = FilerSendFile(pFiler, l->name, target);
	 if (sta != FILE_SEND_OK)
	 {
	    fprintf(USE_OUT, "failed!\n");
            FilerSync(pFiler);
            make_lxdir(pFiler, target);
	    fprintf(USE_OUT, "Trying again.");
            sta = FilerSendFile(pFiler, l->name, target);
            if (sta != FILE_SEND_OK)
	    { 
	       fprintf(USE_OUT, "\nCannot send file.\n");
               FilerSync(pFiler);
	    }
	 }
         fprintf(USE_OUT, "\n");
      }
      l = l->next;
   }
}

/* --------------------------------------------------------------------
                               M A I N
   -------------------------------------------------------------------- */

int main (int argc, char **argv)
{
   int port = 1;
   int speed = DEF_BAUD;
   FILERCOM *pFiler;
   FLCB MyFlCb;
   int i = 1;
   
   fprintf(USE_OUT, "LXCOPY %s by A. Garzotto\n\n", VERSION);
   
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
	  case 'R':
	  case 'r': recursive = 1; break;
	  case 'A':
	  case 'a': archive = 1; break;
	  case 'O':
	  case 'o': overwrite = 0; break;
	  default: help(); break;
	 }
      }
      else if (i < argc - 1)
	 add_list(argv[i], 1, 0);
      else
	 add_list(argv[i], 0, 0);
      i++;      
   }
   if (!inlist || !outlist) help();
   
   /* replace default (PAL) FlCb handler by one of our own */
   MyFlCb = FlCb;
   MyFlCb.FlcbSendBlock = MySendBlock;
   MyFlCb.FlcbRecvBlock = MyRecvBlock;

   if(!(pFiler = FilerConnect(port, speed, &MyFlCb))) {
      fprintf(USE_OUT, "\nUnable to connect to palmtop!\n");
      exit(EXITVAL(1));
   }
   
   if (strchr(outlist->name, ':'))
      from_linux(pFiler);
   else
      from_lx(pFiler);
   
   FilerDisconnect(pFiler);
   exit(EXITVAL(0));
}

