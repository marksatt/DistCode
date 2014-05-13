# Makefile for the Simple Sockets Library (C version) Executables
#
#  NASA --   Goddard Space Flight Center, Code 714.1
#            Intelligent Robotics Laboratory
#            Robotics Branch
#
#  Authors:  Dr. Charles E. Campbell, Jr.
#            Terry McRoberts
#

# list of executables
EXES= Spm sktdbg spmtable srmsrvr spmchk

# to convert C into executables
.c:
	$(CC) $(CFLAGS) $(LDFLAGS) $< $(LOADLIBES) -o $(EXETGT)/$@

all : $(EXES)

clean :
	/bin/rm -f *.o *.bak *.err Spm sktdbg spmchk spmtable srmsrvr
