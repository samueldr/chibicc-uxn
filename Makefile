CFLAGS=-std=c11 -g -static -fno-common
SRCS=codegen.c main.c parse.c tokenize.c type.c
OBJS=$(SRCS:.c=.o)

chibicc: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

$(OBJS): chibi.h

# TODO: support uxn chibicc compiling itself?
#chibicc-gen2: chibicc $(SRCS) chibi.h
#	./self.sh

# TODO: support extern tests, maybe via an uxntal file
#extern.o: tests-extern
#	gcc -xc -c -o extern.o tests-extern

test: chibicc #extern.o
	cc -I. -P -E -x c tests -o tmp-tests.c
	./chibicc tmp-tests.c > tmp-tests.tal
	uxnasm tmp-tests.tal tmp-tests.rom
	uxncli tmp-tests.rom

#test-gen2: chibicc-gen2 extern.o
#	./chibicc-gen2 tests > tmp.s
#	gcc -static -o tmp tmp.s extern.o
#	./tmp

clean:
	rm -rf chibicc chibicc-gen* *.o *~ tmp*

.PHONY: test clean
