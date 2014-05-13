# Makefile for the Simple Sockets Library (C version)
#
#  Authors:  Charles E. Campbell, Jr., Ph.D.
#            Terry McRoberts
#
#
# Building the SSL:
#
# Some Unix (tm) boxes need to run a program called "ranlib" (ex. Sun)
# You will notice below a "#ranlib ..." line; if your machine needs it,
# remove the "#".
#
# Please modify the Makefile below for the following exceptions:
#
# Compiler Exceptions     Loader Exceptions
#                              SCO
#                              Solaris 2.1
#
# You may also wish to set the "RDCOLOR" environment variable in your
# login/profile/startup-sequence/etc file to be the string where the
# <rdcolor.dat> file is to be located.  Furthermore, you may wish to
# modify the <rdcolor.dat> file to include escape sequences for your
# favorite terminal(s) (should be fairly obvious by inspection how to
# do so using some editor).
#
# Option: The PortMaster can optionally use a "firewall" to keep unwanted
#         visitors out.  The PortMaster (Spm) uses an environment variable
#         (SPMFIREWALL) to specify where the firewall file is located.
#         One may also start up the PortMaster (Spm -f firewallfilename).
#         Please read the documentation on this.

CC = cc

OBJ = Saccept.o      Sprintf.o      Stest.o        outofmem.o     \
      Sclose.o       Sprtskt.o      Stimeoutwait.o rdcolor.o      \
      Sgets.o        Sputs.o        Svprintf.o     sprt.o         \
      Smaskwait.o    Sread.o        Swait.o        srmtrblk.o     \
      Smkskt.o       Sreadbytes.o   Swrite.o       stpblk.o       \
      Sopen.o        Srmsrvr.o      error.o        stpnxt.o       \
      Sopenv.o       Sscanf.o       fopenv.o       strnxtfmt.o    \
      Speek.o        cprt.o         Speername.o    Speeraddr.o    \
	  Sinit.o

# LIB: name of library
LIB       = simpleskts.a

# EXETGT: where to put executables, as seen from the viewpoint of the EXE
#  subdirectory
EXETGT=../EXE

# usual compiler stuff
CFLAGS    = -O -I./HDR
LOADLIBES = ../$(LIB)

# for SCO UNIX:
# LOADLIBES = ../$(LIB) -lsocket

# for Solaris 2.1
# LOADLIBES = ../$(LIB) -lnsl

# for g++ compiler: (tnx to Gary Johnson)
# CFLAGS= -Wall -ansi -pedantic -O -I./HDR

# all     : $(LIB) exes examples
all     : $(LIB)

$(LIB)  : $(OBJ)
	ar r simpleskts.a *.o
#	ranlib simpleskts.a
#	/bin/rm -f *.o
	@echo "finished updating simpleskts.a"

exes : $(LIB)
	( cd EXE;                                                  \
      make CC=$(CC) LIB=$(LIB) CFLAGS="-O -I../HDR" LOADLIBES=$(LOADLIBES) EXETGT=$(EXETGT) )

examples : $(LIB)
	( cd EXAMPLES;                                             \
      make CC=$(CC) LIB=$(LIB) CFLAGS="-O -I../HDR" LOADLIBES=$(LOADLIBES) )

distclean clean   :
	@/bin/rm -f *.a *.bak *.err $(OBJ)
	(cd EXE; make clean )
	(cd EXAMPLES ; make clean )
	@echo "All object files, the simpleskts library, and executables are now gone"

install :
