all: pciscan32 clean
pciscan32: *.o
	gcc -m32 -Wall -static *.o -o pciscan32
*.o: *.c
	gcc -m32 -Wall -static -c *.c
clean:
	rm -f *.o

