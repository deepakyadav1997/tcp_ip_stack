gcc -g -c glthread.c -o glthread.o
gcc -g -c testapp.c -o testapp.o
gcc -g testapp.o glthread.o -o exe