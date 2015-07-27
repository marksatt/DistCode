cl -c *.c /AS 
lib @makelib
cd  exe
cl -c /AS -I\c600\socket *.c
cd ..
link exe\sktdbg ,,,smplskts+socket+slibce+winapp,/noi /noe
link exe\spmtable,,,smplskts+socket+slibce+winapp,/noi /noe
link exe\srmsrvr ,,,smplskts+socket+slibce+winapp,/noi /noe
mv *.exe c:\c600\socket\exe
