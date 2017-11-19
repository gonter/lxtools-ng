/* $Id: lxbackup.c,v 1.2 2000/10/09 00:08:02 gonter Exp $ */
/* --------------------------------------------------------------------
	Project: Communication package Linux-HPx00LX Filer
	Author:  Edward A. Falk
	Started: 12 Mar 97
	Subject: HP95/HP100/HP200 incremental backup utility
  -------------------------------------------------------------------- */

/* -------------------------------------------------------------------
	Modifications by Peter Watkins
	November 16, 1997
	- added support for LXTOOLS_STDOUT
	- added "-c" switch (and added notes to man page)
	- removed hard-coded exclude rule
	- corrected notReally error in restore code
   ------------------------------------------------------------------- */

/* -------------------------------------------------------------------
	Modifications by Peter Watkins
	January 1, 2000
	- changed date functions to assume that -since dates < 80
	  or DOS timestamps with years < 80 are 21st century
   ------------------------------------------------------------------- */


static	char	*usage =

"Backup files on hp95, hp100 or hp200, via Filer\n"
"Usage: lxbackup [options] [dir...]\n"
"\n"
"	<dir> <dir>...		disks & directories to back up (C:\\)\n"
"\n"
"	-all			all files.\n"
"	-since <mmddhhmm[yy]>	all files modified since date\n"
"	-newer <filename>	all files newer than specified file\n"
"	-a			all files that have the archive bit set\n"
"	-1|2|3|4		set com port (1)\n"
"	-d <devname>		set full com port name\n"
"	-b <baud>		set baud rate (38400)\n"
"	-l			list remote files only\n"
"	-n			Show what would be done only.\n"
"	-c			Use \"-\" instead of \":\" for local files\n"
"	-v			verbose\n"
"	-restore		restore instead of backup\n"
"	-q|h|?			this message\n"
"	-exclude <pat...>	exclude files matching patterns (must be last)\n"
"\n"
" Return values:\n"
"	0 = success\n"
"	1 = failure\n"
"	2 = user error\n"
"	3 = internal error\n"
	;

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <ftw.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/stat.h>

#include "pal.h"
#include "config.h"

#define	NAMESIZE	13		/* Dos filenames limited to 12 chars */
#define	MAXDIRS		256		/* arbitrary. */

extern	int	sys_nerr ;
/* commented out for LXTools 1.1c
extern	char	*sys_errlist[] ;
*/

static	char	*excludes[1024] ;
static	int	nexcludes = 0 ;

static	time_t	since, parseDate(), fileTime() ;
static	int	archive = 0 ;
static	int	verbose = 0 ;
static	int	port = 1 ;
static	char	*dev_name = NULL ;
static	int	baud = 38400 ;
static	int	listOnly = 0 ;
static	char	*top[MAXDIRS] ;
static	int	ntop = 0 ;
static	int	compress = 0 ;
static	int	restore = 0 ;
static	int	notReally = 0 ;
static	int	noColon = 0 ;


typedef	struct stackedName {
	  char	name[NAMESIZE] ;
	  FDATESTAMP date ;
	  unsigned long size ;
	  struct stackedName	*next ;
	} StackedName ;

#define	strmatch(a,b)	(strcmp(a,b)==0)
#define	MALLOC(t,n)	((t *)malloc(sizeof(t)*(n)))

/* commented out for LXTools 1.1c
extern	int	putenv(char *) ;
*/

	/* forward references */


static	int	BackupDir(FILERCOM *, char *dirName) ;
static	int	RestoreDir(FILERCOM *, char *dirName) ;
static	int	RestoreCB(const char *, const struct stat *, int) ;
static	int	RestoreFile(FILERCOM *, const char *, const struct stat *) ;
static	int	CreateDir(FILERCOM *pFiler, const char *name) ;
static	int	makedir(char *) ;
static	int	CheckFile(FILERCOM *pFiler, char *name) ;
static	int	ExcludeFile(const char *name) ;
static	int	BackupFile(FILERCOM *pFiler, char *name, StackedName *) ;
static	int	makePath(char *name) ;
static	time_t	FDate2Unix(FDATESTAMP *) ;
static	void	dos2unix(const char *dos, char *unx) ;
static	void	unix2dos(const char *unx, char *dos) ;
static	const char	*lxbasename(const char *) ;
static	void	die(char *msg, char *msg2, int rcode) ;
static	void	colonToDash(const char *unx, char *dos) ;


int
main(int argc, char **argv)
{
	int	i,j ;
	char	topDir[MAXPATHLEN] ;
	char	ttyEnvBuf[256] ;
	FILERCOM *pFiler ;
	int	rval = 0 ;
	char	goodName[MAXPATHLEN] ;

	/* couldn't see that it respected LXTOOLS_BAUD */	
	if(getenv("LXTOOLS_BAUD")) {
	  if(atoi(getenv("LXTOOLS_BAUD"))) {
	    baud = atoi(getenv("LXTOOLS_BAUD"));
	  } 
	}

	since = -1 ;

	while(--argc > 0)
	{
	  ++argv ;
	  if( **argv == '-' )
	  {
	    if( strmatch(*argv, "-since") ) {	/* mmddhhmm[yy] format */
	      if( --argc > 0 )
		since = parseDate(*++argv) ;
	      else
		die("-since requires a date",NULL,2) ;
	    }
	    else if( strmatch(*argv, "-newer") ) {
	      if( --argc > 0 )
		since = fileTime(*++argv) ;
	      else
		die("-newer requires a file name",NULL,2) ;
	    }
	    else if( strmatch(*argv, "-all") )
	      since = 0 ;
	    else if( strmatch(*argv, "-a") )
	      archive = 1 ;
	    else if( strmatch(*argv, "-v") )
	      ++verbose ;
	    else if( strmatch(*argv, "-restore") )
	      restore = 1 ;
	    else if( strmatch(*argv, "-z") )
	      compress = 1 ;
	    else if( strmatch(*argv, "-l") )
	      listOnly = 1 ;
	    else if( strmatch(*argv, "-n") )
	      verbose = notReally = 1 ;
	    else if( strmatch(*argv, "-c") )
	      noColon = 1 ;
	    else if( isdigit(argv[0][1]) )
	      port = atoi(&argv[0][1]) ;
	    else if( strmatch(*argv, "-d") && --argc > 0 ) {
	      dev_name = *++argv ;
	      sprintf(ttyEnvBuf, "LXTOOLS_LINE=%s", dev_name) ;
	      putenv(ttyEnvBuf) ;
	    }
	    else if( strmatch(*argv, "-b") && --argc > 0 )
	      baud = atoi(*++argv) ;

	    else if( strmatch(*argv, "-exclude") )
	    {
	      --argc ; ++argv ;
	      while( argc > 0 && **argv != '-' )
	      {
		char *ptr ;
		excludes[nexcludes++] = ptr = *argv++ ;
		for( ;*ptr != '\0'; ++ptr )
		  *ptr = toupper(*ptr) ;
		--argc ;
	      }
	    }

	    else if(strmatch(*argv,"-?") || strmatch(*argv,"-h") ||
		    strmatch(*argv,"-q") )
	      die(usage,NULL,0) ;

	    else {
	      fprintf(USE_OUT, "unknown option: %s\n",*argv) ;
	      die(usage,NULL,2) ;
	    }

	  }
	  else if( ntop < MAXDIRS )
	    top[ntop++] = *argv ;
	  else {
	    fprintf(USE_OUT,
	      "lxbackup: too many directories specified (max %d), %s ignored\n",
	      MAXDIRS, *argv) ;
	    rval = 3 ;
	  }
	}

	if( verbose >= 2 ) fprintf(USE_OUT,"connecting to remote...") ;
	if( !(pFiler = FilerConnect(port, baud, &FlCb)) )
	  die("failed to connect to remote",NULL,1) ;
	if( verbose >= 2 ) fprintf(USE_OUT,"connected\n") ;

	if( ntop == 0 )
	  top[ntop++] = "C:\\" ;


	for(i=0; i<ntop; ++i)
	{
	  if( !restore )
	  {
	    strncpy(topDir, top[i], sizeof(topDir)) ;
	    makedir(topDir) ;
	    if( (j = BackupDir(pFiler, topDir)) )
	      rval = j ;
	  }
	  else
	  {
	    colonToDash(top[i],goodName);
/*
	    fprintf(USE_OUT,"passing \"%s\" to RestoreDir\n",goodName);
*/
	    if( (j = RestoreDir(pFiler, goodName)) )
	      rval = j ;
	  }
	}


	if( verbose >= 2 ) fprintf(USE_OUT,"disconnecting ...") ;
	FilerDisconnect(pFiler) ;
	if( verbose ) fprintf(USE_OUT, "done\n") ;

	exit(EXITVAL(rval)) ;
}



	/* convert a directory name to "canonical" form.  That is,
	 * all '/' converted to '\', and a trailing '\' added if needed.
	 */

static	int
makedir(char *dir)
{
	char *p ;

	for(p = dir; *p; ++p )
	  if( *p == '/' ) *p = '\\' ;
	if( p[-1] != '\\' ) *p++ = '\\' ;
	return p - dir ;
}



	/* This is the heart of the backup procedure.  Given a directory
	 * name, ask for a listing of that directory.  All files are
	 * tested with CheckFile(), all directories are stacked up.
	 *
	 * Once the listing is complete, all flagged files passed to
	 * BackupFile(), all directories are passed (recursively) to
	 * BackupDir()
	 *
	 * Input name must always have three extra bytes at the end so
	 * that this function may append "*.*".  Input name must be in
	 * canonical form, described in makedir().
	 */

static	int
BackupDir(FILERCOM *pFiler, char *name)
{
	StackedName	*dirStack = NULL ;
	StackedName	*fileStack = NULL ;
	StackedName	*namePtr ;
	int		len = strlen(name) ;
	int		status ;
	int		rval = 0 ;

	if( len > MAXPATHLEN-5 ) {
	  fprintf(USE_OUT, "Filename too long: %s\n", name) ;
	  return 3 ;
	}

	strcpy(name+len, "*.*") ;
	status = FilerAskDir(pFiler, name);
	name[len] = '\0' ;

	switch( status ) {
	 case NO_RESPONSE:
	    fprintf(USE_OUT, "Server not responding.\n") ;
	    return 1 ;
	 case BAD_REQUEST:
	    fprintf(USE_OUT, "Bad request.\n") ;
	    return 1 ;
	 default:
	    fprintf(USE_OUT, "Transmission error.\n") ;
	    return 1 ;
	 case SERVER_ACK:
	    break ;
	}

	if( listOnly )
	  printf("\nDirectory of %s:\n\n", name) ;
	else if( verbose >= 2 )
	  fprintf(USE_OUT, "Searching directory %s:\n", name) ;

	while( FilerGetDir(pFiler) != CANNOT_GET_ENTRY )
	{
	  if( pFiler->Attribute & 0x10 )
	  {
	    /* It's a directory.  Save the name for later. */
	    if( strcmp((char *)pFiler->Name, ".") != 0  &&
	        strcmp((char *)pFiler->Name, "..") != 0 )
	    {
	      namePtr = MALLOC(StackedName, 1) ;
	      if( namePtr == NULL ) {
		fprintf(USE_OUT, "lxbackup: out of memory\n") ;
		return 3 ;
	      }

	      strcpy(namePtr->name, (char *)pFiler->Name) ;
	      namePtr->next = dirStack ;
	      dirStack = namePtr ;
	    }

	    if( listOnly )
	      printf("%-12s    <DIR>      %02d-%02d-%02d  %02d:%02d\n",
		pFiler->Name,
		pFiler->DateStamp.month, pFiler->DateStamp.day,
		pFiler->DateStamp.year+80,
		pFiler->DateStamp.hour, pFiler->DateStamp.min);
	  }
	  else
	  {
	    /* It's a file.  See if it needs to be backed up. */
	    if( listOnly )
	      printf("%-12s %12lu  %02d-%02d-%02d  %02d:%02d\n",
		pFiler->Name, pFiler->FileSize,
		pFiler->DateStamp.month, pFiler->DateStamp.day,
		pFiler->DateStamp.year+80,
		pFiler->DateStamp.hour, pFiler->DateStamp.min);
	    else
	    {
	      if( len+strlen((char *)pFiler->Name) >= MAXPATHLEN )
		fprintf(USE_OUT, "File name too long (ignored): %s\\%s\n",
		    name, pFiler->Name) ;
	      else
	      {
		strcpy(name+len, (char *)pFiler->Name) ;
		if( CheckFile(pFiler, name) )
		{
		  namePtr = MALLOC(StackedName, 1) ;
		  if( namePtr == NULL ) {
		    fprintf(USE_OUT, "lxbackup: out of memory\n") ;
		    return 3 ;
		  }

		  strcpy(namePtr->name, (char *)pFiler->Name) ;
		  namePtr->date = pFiler->DateStamp ;
		  namePtr->size = pFiler->FileSize ;
		  namePtr->next = fileStack ;
		  fileStack = namePtr ;
		}
	      }
	    }
	  }
	}


	/* Next: unstack the files and back them up. */

	while( rval == 0 && fileStack != NULL )
	{
	  namePtr = fileStack ;
	  fileStack = fileStack->next ;

	  strcpy(name+len, namePtr->name) ;
	  rval = BackupFile(pFiler, name, namePtr) ;
	  name[len] = '\0' ;

	  free(namePtr) ;
	}


	/* Next: unstack the directories and recurse */

	while( rval == 0 && dirStack != NULL )
	{
	  namePtr = dirStack ;
	  dirStack = dirStack->next ;
	  if( len+strlen(namePtr->name) >= MAXPATHLEN )
	    fprintf(USE_OUT, "Directory name too long (ignored): %s\\%s\n",
		name, namePtr->name) ;
	  else
	  {
	    strcpy(name+len, namePtr->name) ;
	    strcat(name+len, "\\") ;
	    rval = BackupDir(pFiler, name) ;
	    name[len] = '\0' ;
	  }

	  free(namePtr) ;
	}

	return rval ;
}



	/* examine filename and determine if it needs to be backed up */

static	int
CheckFile(FILERCOM *pFiler, char *name)
{
	char	name2[MAXPATHLEN] ;
	time_t	ftime ;

	if( ExcludeFile(name) )
	  return 0 ;

	if( archive )
	  return pFiler->Attribute & 0x20 ;

	dos2unix(name, name2) ;

	if( since != 0 )	/* convert filer date to unix date */
	{
	  ftime = FDate2Unix(&pFiler->DateStamp) ;

	  if( since == -1 )	/* compare file dates */
	  {
	    if( ftime <= fileTime(name2) )
	      return 0 ;
	  }
	  else
	  {
	    if( ftime <= since )
	      return 0 ;
	  }
	}


	if( !notReally && makePath(name2) )
	  return 0 ;

	return 1 ;
}



static	int
ExcludeFile(const char *name)
{
	char	**ptr = excludes ;
	int	len = strlen(name) ;
	int	n = nexcludes ;

/* hard-coded: exclude directories */
	if( strncmp(name+len-1,"\\",4) == 0 )
	  return 1 ;

/* hard-coded: exclude directories, c:\_*.wk1 
	if( strncmp(name+len-1,"\\",4) == 0 ||
	    strncmp(name,"C:\\_",4) == 0 && strncmp(name+len-4,".WK1",4) == 0 )
	  return 1 ;
*/

	/* search exclude list */

	for(; --n >= 0; ++ptr )
	  if( strstr(name, *ptr) != NULL )
	    return 1 ;

	return 0 ;
}



static	int
BackupFile(FILERCOM *pFiler, char *name, StackedName *namePtr)
{
	char	name2[MAXPATHLEN] ;
	struct timeval times[2] ;

	dos2unix(name, name2) ;
	if( verbose )
	  fprintf(USE_OUT, "backup %s, %ld bytes ... ", name, namePtr->size) ;

	if( notReally )
	  fprintf(USE_OUT, "\n") ;
	else
	{
	  if( FilerGetFile(pFiler, name, name2) != GOT_FILE_OK )
	    return 1 ;

	  times[0].tv_sec = times[1].tv_sec = FDate2Unix(&namePtr->date) ;
	  times[0].tv_usec = times[1].tv_usec = 0 ;
	  (void) utimes(name2, times) ;

	  if( compress )
	  {
	    char cmd[MAXPATHLEN] ;
	    strcpy(cmd, "gzip ") ;
	    strncat(cmd, name2, sizeof(cmd)) ;
	    if( verbose ) fprintf(USE_OUT, "compress ... ") ;
	    system(cmd) ;
	  }
	  if( verbose ) fprintf(USE_OUT, "done\n") ;
	}
	return 0 ;
}



	/* This is the heart of the restore procedure.  Given a directory
	 * name, examine that directory and its contents.  For each file
	 * found, see if it exists or is older on the HP.  If so,
	 * write it to the HP.
	 */

static	FILERCOM	*fcom ;	/* stupid ftw() doesn't accept client args */

static	int
RestoreDir(FILERCOM *pFiler, char *name)
{
	fcom = pFiler ;
	ftw(name, RestoreCB, 10) ;
	return 0 ;
}


static	int
RestoreCB(const char *name, const struct stat *sbuf, int what)
{
/*
	fprintf(USE_OUT,"RestoreCB was passed \"%s\"\n",name);
*/
	switch( what ) {
	  case FTW_F:
#ifdef	FTW_SL
	  case FTW_SL:
#endif
	    return RestoreFile(fcom, name, sbuf) ;

	  case FTW_D:
#ifdef	FTW_DP
	  case FTW_DP:
#endif
	    if( verbose >= 2 )
	      fprintf(USE_OUT, "Directory %s:\n", name) ;
	    CreateDir(fcom, name) ;
	    break ;

	  case FTW_DNR:
	    fprintf(USE_OUT, "directory %s unreadable\n", name) ;
	    break ;
	  case FTW_NS:
#ifdef	FTW_SLN
	  case FTW_SLN:
#endif
	    fprintf(USE_OUT, "file %s unreadable\n", name) ;
	    break ;
	}
	return 0 ;
}



	/* conditionally create a directory on the HP */

static	int
CreateDir(FILERCOM *pFiler, const char *name)
{
	char	dosname[MAXPATHLEN] ;
	const char	*dirname ;
	int	needCreate = 1 ;

	if( strlen(name) == 2 && name[1] == ':' )
	  return 0 ;

	unix2dos(name, dosname) ;
	dirname = lxbasename(dosname) ;

	if( FilerAskDir(pFiler, dosname) != SERVER_ACK )
	  return 1 ;

	while( FilerGetDir(pFiler) == GOT_DIR_ENTRY )
	  if( strmatch((char *)pFiler->Name, dirname) )
	  {
	    if( pFiler->Attribute & 0x10 )
	      needCreate = 0 ;
	    else
	      fprintf(USE_OUT, "Cannot create directory %s on top of file\n",
		dosname) ;
	  }

	if( needCreate )
	{
	  if( verbose ) fprintf(USE_OUT, "Create directory %s\n", dosname) ;
	  if( !notReally )
	    FilerMakeDir(pFiler, dosname) ;
	}

	return 0 ;
}


static	int
RestoreFile(FILERCOM *pFiler, const char *name, const struct stat *sbuf)
{
	char	dosname[MAXPATHLEN] ;
	const char	*filename ;
	int	needRestore = 1 ;
	int	foundit = 0 ;

	if( ExcludeFile(name) )
	  return 0 ;

	unix2dos(name, dosname) ;
	filename = lxbasename(dosname) ;

/*
	fprintf(USE_OUT,"dosname is \"%s\" and lxbasename is \"%s\"\n",dosname,filename);
	fprintf(USE_OUT,"RestoreFile was passed \"%s\"\n",name);
*/


	/* Policy:  If user entered "-all" (since = 0), update all files
	 * unconditionally.  If user specified a date (-since or -newer),
	 * update all files newer than the specified date.  Else, find out
	 * the date of the file on the HP (if any) and update if the
	 * local copy is newer.
	 */


	/* first, see if exists, and get its age */

	if( FilerAskDir(pFiler, dosname) != SERVER_ACK )
	  return 1 ;

	while( FilerGetDir(pFiler) == GOT_DIR_ENTRY )
	{
	  if( strmatch((char *)pFiler->Name, filename) )
	  {
/*
	    fprintf(USE_OUT,"foundit\n");
*/
	    foundit = 1 ;
	  }
	}

	if( since != -1 )
	{
	  if( sbuf->st_mtime <= since )
	    needRestore = 0 ;
	}
	else if( foundit && sbuf->st_mtime <= FDate2Unix(&pFiler->DateStamp) )
	  needRestore = 0 ;

	if( needRestore && foundit )
	{
	  if( verbose >= 2 ) fprintf(USE_OUT, "delete %s ... ", dosname) ;
	  if ( notReally ) 
	    fprintf(USE_OUT, "\n") ;
	  else {
	    if( FilerDelFile(pFiler, dosname) != FILE_DELETED )
	    {
	      fprintf(USE_OUT, "could not delete %s\n", dosname) ;
	      needRestore = 0 ;
	    }
	  } /* only delete if not notReally */
	}

	if( needRestore )
	{
	  if( verbose )
	    fprintf(USE_OUT, "restore %s, %ld bytes ... ", name, (long) sbuf->st_size) ;
	  if( notReally )
	    fprintf(USE_OUT, "\n") ;
	  else
	    switch( FilerSendFile(pFiler, (char *)name, dosname) )
	    {
	      case NO_SOURCE_FILE:
		fprintf(USE_OUT, "cannot open %s\n", name) ;
		break ;
	      case CANNOT_SEND_FNAME:
		fprintf(USE_OUT, "cannot send %s\n", dosname) ;
		break ;
	      case NO_RESPONSE:
		fprintf(USE_OUT, "server does not respond\n") ;
		break ;
	      case FILE_SEND_OK:
		if( verbose ) fprintf(USE_OUT, "done\n") ;
		break ;
	    }
	}

	return 0 ;
}



/* build the directory path necessary to house this file */


static	int
mkdir2(char *path, int mode)
{
	char	*last ;

	if( access(path, W_OK|X_OK) == 0 )
	  return 0 ;

	last = strrchr(path,'/') ;
	if( last != NULL ) {
	  *last = '\0' ;
	  mkdir2(path,mode) ;
	  *last = '/' ;
	}
	return mkdir(path,mode) ;
}

static	int
makePath(char *name)
{
	char	path[256], *last ;

	strcpy(path,name) ;

	if( (last = strrchr(path, '/')) == NULL )
	  return 0 ;

	*last = '\0' ;

	if( access(path, W_OK|X_OK) == 0 )
	  return 0 ;

	if( mkdir2(path, 0775) == 0 )
	  return 0 ;

	fprintf(USE_OUT, "lxbackup: unable to make directory %s, %s\n",
		path, sys_errlist[errno]) ;

	return -1 ;
}






	/* convert dos-style name to unix-style name */

static	void
dos2unix(const char *dos, char *unx)
{
	const char	*ptr = dos ;
	char		*ptr2 = unx ;

	while(*ptr != '\0' )
	{
	  if( *ptr == '\\' ) {
	    *ptr2++ = '/' ;
	  }
	  else if ( (noColon == 1) && ( *ptr == ':') ) {
	    *ptr2++ = '-' ;
	  }

	  else
	    *ptr2++ = tolower(*ptr) ;

	  ++ptr ;
	}
	*ptr2 = '\0' ;
}


static	void
colonToDash(const char *unx, char *dos)
{
	const char	*ptr = unx ;
	char		*ptr2 = dos ;
	int		unix2dosCount = 0;

	while(*ptr != '\0' )
	{
	  ++unix2dosCount ;
	  if ( (noColon == 1) && (*ptr == ':') && (unix2dosCount == 2) ) {
	    *ptr2++ = '-' ;
	  }

	  else
	    *ptr2++ = *ptr ;

	  ++ptr ;
	}
	*ptr2 = '\0' ;
}

	/* convert unix-style name to dos-style name */

static	void
unix2dos(const char *unx, char *dos)
{
	const char	*ptr = unx ;
	char		*ptr2 = dos ;
	int		unix2dosCount = 0;

	while(*ptr != '\0' )
	{
	  ++unix2dosCount ;
	  if( *ptr == '/' ) {
	    *ptr2++ = '\\' ;
	  }
	  else if ( (noColon == 1) && (*ptr == '-') && (unix2dosCount == 2) ) {
	    *ptr2++ = ':' ;
	  }

	  else
	    *ptr2++ = toupper(*ptr) ;

	  ++ptr ;
	}
	*ptr2 = '\0' ;
}

	/* examine filename and determine if it needs to be backed up */

static	time_t
FDate2Unix(FDATESTAMP *fdate)
{
	struct tm tm ;

	tm.tm_sec = fdate->sec ;
	tm.tm_min = fdate->min ;
	tm.tm_hour = fdate->hour ;
	tm.tm_mday = fdate->day ;
	tm.tm_mon = fdate->month - 1 ;
	tm.tm_year = fdate->year + 80 ;
	tm.tm_wday = tm.tm_yday = tm.tm_isdst = 0 ;
#ifdef	__STDC__
	return mktime(&tm) ;
#else
	return timelocal(&tm) ;
#endif
}


static	const	char	*
lxbasename(const char *name)
{
	const char	*rval = strrchr(name, '\\') ;
	return rval == NULL ? name : rval+1 ;
}



	/* mmddhhmm[yy] format */

time_t
parseDate(char *date)
{
	time_t		temp ;
	struct tm	tm, *tmptm ;
	int		idate = atoi(date) ;

	temp = time(NULL) ;
	tmptm = localtime(&temp) ;
	tm = *tmptm ;

	if( idate > 99999999 ) {
	  tm.tm_year = idate % 100 ; idate /= 100 ;
	}

	/* if year < 80 (we *are* dealing with DOS, here!)
	   assume it's 21st century -PW */
	if ( tm.tm_year < 80 ) tm.tm_year += 100;

	tm.tm_min = idate % 100 ; idate /= 100 ;
	tm.tm_hour = idate % 100 ; idate /= 100 ;
	tm.tm_mday = idate % 100 ; idate /= 100 ;
	tm.tm_mon = (idate % 100) - 1 ;

#ifdef	__STDC__
	return mktime(&tm) ;
#else
	return timelocal(&tm) ;
#endif
}


time_t
fileTime(char *file)
{
	struct stat	buf ;

	if( stat(file, &buf) ) 
	  return 0 ;
	return buf.st_mtime ;
}


static	void
die(char *msg, char *msg2, int rcode)
{
	if( msg != NULL )
	  fprintf(USE_OUT, "lxbackup: %s\n", msg) ;
	if( msg2 != NULL )
	  fprintf(USE_OUT, "%s\n", msg2) ;
	exit(EXITVAL(rcode)) ;
}
