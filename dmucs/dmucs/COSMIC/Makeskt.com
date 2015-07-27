$! makeskt.com: this is the command script for VMS machines
$!
$! Simple Sockets Library Authors: Dr. Charles E. Campbell, Jr.
$!                                 Terry McRoberts
$!
$! This com script assumes that you have already placed all of the
$! files in the target directory with subdirectories
$! EXE.DIR  HDR.DIR  EXAMPLES.DIR
$!
$ define lnk$library sys$library:vaxcrtl
$ dr:='F$DIRECTORY()
$ assign 'dr'     sockets:
$ set def [.HDR]
$ dr:='F$DIRECTORY()
$ assign 'dr' C$INCLUDE
$ set def [-]
$!
$! *** Library Generation ***
$!
$ write sys$output "Begin Simple Sockets Library compilation"
$ cc Saccept.c
$ cc Sclose.c
$ cc Sgets.c
$ cc Sinit.c
$ cc Smaskwait.c
$ cc Smkskt.c
$ cc Sopen.c
$ cc Sopenv.c
$ cc Speek.c
$ cc Speername.c
$ cc Sprintf.c
$ cc Sprtskt.c
$ cc Sputs.c
$ cc Sread.c
$ cc Sreadbytes.c
$ cc Srmsrvr.c
$ cc Sscanf.c
$ cc Stest.c
$ cc Stimeoutwait.c
$ cc Svprintf.c
$ cc Swait.c
$ cc Swrite.c
$ cc error.c
$ cc fopenv.c
$ cc outofmem.c
$ cc rdcolor.c
$ cc sprt.c
$ cc srmtrblk.c
$ cc stpblk.c
$ cc stpnxt.c
$ cc strnxtfmt.c
$!
$ write sys$output "Creating Simple Sockets Library now"
$ lib/create simpleskts *.obj
$ delete *.obj;*
$ set def [.EXE]
$!
$! *** Executable Generation ***
$!
$ write sys$output "Compiling sktdbg"
$ cc sktdbg.c
$ write sys$output "Linking sktdbg"
$ link sktdbg,sockets:simpleskts/lib,sys$library:ucx$ipc/lib
$!
$ write sys$output "Compiling spm"
$ cc spm.c
$ write sys$output "Linking spm"
$ link spm,sockets:simpleskts/lib,sys$library:ucx$ipc/lib
$!
$ write sys$output "Compiling spmtable"
$ cc spmtable.c
$ write sys$output "Linking spmtable"
$ link spmtable,sockets:simpleskts/lib,sys$library:ucx$ipc/lib
$!
$ write sys$output "Compiling srmsrvr"
$ cc srmsrvr.c
$ write sys$output "Linking srmsrvr"
$ link srmsrvr,sockets:simpleskts/lib,sys$library:ucx$ipc/lib
$ set def [-]
$ write sys$output "The Simple Sockets Library is now Available"
