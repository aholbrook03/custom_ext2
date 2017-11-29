CC = gcc
AR = ar
CFLAGS = -Wall -O2
ARFLAGS = cr

default: cat ls_l

cat: cat.c libext2.a
	${CC} ${CFLAGS} -o $@ $^

ls_l: ls_l.c libext2.a
	${CC} ${CFLAGS} -o $@ $^

libext2.a: ext2.o
	${AR} ${ARFLAGS} $@ $^

ext2.o: ext2.c ext2.h
	${CC} ${CFLAGS} -c -o $@ $<

.PHONY: clean
clean:
	rm -f libext2.a ext2.o cat ls_l
