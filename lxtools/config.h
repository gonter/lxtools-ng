/* --------------------------------------------------------------------
   Project: LX tools
   Module:  config.h
   Author:  A. Garzotto
   Started: 30. Nov. 95
   Last Modified: 13. Dec. 95
   Subject: configuration specific  include file
   -------------------------------------------------------------------- */

/* This file contains constants that are system specific */


/* Set this to reflect the base name of the serial port devices */
/* Sun: "/dev/tty" */
/* HP-UX: "/dev/tty1p" (???) */
/* Linux: "/dev/ttyS" most modern distributions, others use /dev/cua */

#ifdef _AIX
#define SERIAL_NAME "/dev/tty"
#define DEF_BAUD 19200            /* Maybe this could be faster ... */
#endif

#ifdef __FreeBSD__
#define SERIAL_NAME "/dev/cuaa" /* cuaa<n> or ttyd<n> ?? */
#define DEF_BAUD 9600            /* Maybe this could be faster ... */
#endif

#ifdef __NetBSD__
#define SERIAL_NAME "/dev/tty0" /* cuaa<n> or ttyd<n> ?? */
#define DEF_BAUD 9600            /* Maybe this could be faster ... */
#endif

#ifndef SERIAL_NAME
#define SERIAL_NAME "/dev/ttyS"
#endif /* SERIAL_NAME */


/* Set this to reflect the name of first serial device */
/* It will be concatenated with SERIAL_NAME to get the full */
/* name of the device. (on a Sun, this is 'a') */

#define SERIAL_FIRST '1'


/* If your system is not POSIX compliant, comment "#define POSIX" */
/* and uncomment "STTY_COMMAND" instead. You might have to play */
/* with the parameters for the stty command. %ld will be */
/* replaced by the baud rate and %s with the device name */
/* Note that on some systems (e.g. Sun) stty works on the */
/* standard output and not on the standard input. Thus, it */
/* would be "stty sane raw pass8 %ld >%s". */

#define POSIX

/* #define STTY_COMMAND "stty sane raw pass8 %ld <%s" */

/* Set this to the desired default baud rate */

#ifndef DEF_BAUD
#define DEF_BAUD 38400
#endif /* DEF_BAUD */

/* Define this if files copied from the LX to the Unix system */
/* should have lower case names */

#define LOWERCASE

/* This is the version number */

#define VERSION "1.1e"

/* Macro to enable all output to go to STDERR */
#define USE_OUT ( getenv("LXTOOLS_STDOUT") ? stdout : stderr )

/* Macro to make all programs give an exit code of "0" */
#define EXITVAL(y)	( getenv("LXTOOLS_STDOUT") ? 0 : y)

