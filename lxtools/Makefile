#
# FILE Makefile
#
# $Id: Makefile,v 1.3 2002/11/19 04:08:02 gonter Exp $
#

CC = gcc -Wall
INC = pal.h palfiler.h palpriv.h config.h
BINS = lxdir lxcopy lxdel lxmkdir lxrmdir lxbackup
LIB= lxtools.a

all : $(BINS)

clean:
	/bin/rm -f *.o core lxtools.a $(BINS)

reallyclean: distclean
distclean:
	/bin/rm -f *.o core $(BINS) $(LIB)

# FreeBSD doesn't have ftw()
FTW_PATH= ../support/ftw/
FreeBSD NetBSD:
	-@ln -s $(FTW_PATH)ftw.h
	$(CC) -c -o ftw.o -DHAVE_DIRENT_H -DHAVE_LSTAT -DHAVE_D_NAMLEN \
              $(FTW_PATH)ftw.c
	ar ru $(LIB) ftw.o

# ----------------------------------------------------------------------------
OBJ_BASE= filer.o filercb.o asdfiler.o gtdfiler.o synfiler.o \
  delfiler.o rmdfiler.o mkdfiler.o getfiler.o sndfiler.o
$(LIB) : $(OBJ_BASE)
	ar ru $(LIB) $(OBJ_BASE)

# ----------------------------------------------------------------------------
lxdir : lxdir.o $(LIB)
	$(CC) -o lxdir lxdir.o $(LIB)

lxdel : lxdel.o $(LIB)
	$(CC) -o lxdel lxdel.o $(LIB)

lxrmdir : lxrmdir.o $(LIB)
	$(CC) -o lxrmdir lxrmdir.o $(LIB)

lxmkdir : lxmkdir.o $(LIB)
	$(CC) -o lxmkdir lxmkdir.o $(LIB)

lxcopy : lxcopy.o $(LIB)
	$(CC) -o lxcopy lxcopy.o $(LIB)

lxbackup : lxbackup.o $(LIB)
	$(CC) -o lxbackup lxbackup.o $(LIB)

# ----------------------------------------------------------------------------
lxbackup.o : lxbackup.c $(INC)
	$(CC) -c -I. lxbackup.c

lxdir.o : lxdir.c $(INC)
	$(CC) -c lxdir.c

lxcopy.o : lxcopy.c $(INC)
	$(CC) -c lxcopy.c

filer.o : filer.c $(INC)
	$(CC) -c filer.c

filercb.o : filercb.c $(INC)
	$(CC) -c filercb.c

asdfiler.o : asdfiler.c $(INC)
	$(CC) -c asdfiler.c

gtdfiler.o : gtdfiler.c $(INC)
	$(CC) -c gtdfiler.c

synfiler.o : synfiler.c $(INC)
	$(CC) -c synfiler.c

getfiler.o : getfiler.c $(INC)
	$(CC) -c getfiler.c

sndfiler.o : sndfiler.c $(INC)
	$(CC) -c sndfiler.c

delfiler.o : delfiler.c $(INC)
	$(CC) -c delfiler.c

mkdfiler.o : mkdfiler.c $(INC)
	$(CC) -c mkdfiler.c

rmdfiler.o : rmdfiler.c $(INC)
	$(CC) -c rmdfiler.c

