@echo OS/2 Command file:    makes the Simple Sockets Library from a
@echo                       OS/2S console
@echo         Trying Watcomm C++  11.0a or better
@echo        ------------------------------------
@echo Authors: Charles E. Campbell, Jr.
@echo         Terry McRoberts
@echo   mod for OS/2 Steve Richards
@echo ---------------------------------------------------------------

@echo Compiling Simple Sockets Library

@del *.lib >NUL
wmake

rem Compile and link SSL utilities and PortMaster (Spm)
@echo Compiling and linking SSL utilities
cd EXE
@del exe\*.exe
wmake -f MAKEFILE.os2
cd ..

@echo ---------------------------------------------------------------
@echo Utilities and the PortMaster (Spm) are in COSMIC\*.exe
@echo The Simple Sockets library is in COSMIC\smplskts.lib
@echo Try out the examples, too!
@echo ..........................
@echo Best of luck,
@echo Charles E. Campbell
