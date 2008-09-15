targets: curve25519-donna.a curve25519-donna-x86-64.a curve25519-donna-c64.a

test: test-donna test-donna-x86-64 test-donna-c64

clean:
	rm -f *.o *.a *.pp test-curve25519-donna test-curve25519-donna-x86-64 speed-curve25519-donna speed-curve25519-donna-x86-64 test-sc-curve25519-donna-x86-64

curve25519-donna.a: curve25519-donna.o
	ar -rc curve25519-donna.a curve25519-donna.o

curve25519-donna.o: curve25519-donna.c
	gcc -O2 -c curve25519-donna.c -Wall

curve25519-donna-c64.a: curve25519-donna-c64.o
	ar -rc curve25519-donna-c64.a curve25519-donna-c64.o

curve25519-donna-c64.o: curve25519-donna-c64.c
	gcc -O2 -c curve25519-donna-c64.c -Wall

curve25519-donna-x86-64.a: curve25519-donna-x86-64.o curve25519-donna-x86-64.s.o
	ar -rc curve25519-donna-x86-64.a curve25519-donna-x86-64.o curve25519-donna-x86-64.s.o

curve25519-donna-x86-64.s.o: curve25519-donna-x86-64.s.pp
	as -o curve25519-donna-x86-64.s.o curve25519-donna-x86-64.s.pp

curve25519-donna-x86-64.s.pp: curve25519-donna-x86-64.s
	cpp curve25519-donna-x86-64.s > curve25519-donna-x86-64.s.pp

curve25519-donna-x86-64.o: curve25519-donna-x86-64.c
	gcc -O2 -c curve25519-donna-x86-64.c -Wall

test-donna: test-curve25519-donna
	./test-curve25519-donna | head -123456 | tail -1

test-donna-x86-64: test-curve25519-donna-x86-64
	./test-curve25519-donna-x86-64 | head -123456 | tail -1

test-donna-c64: test-curve25519-donna-c64
	./test-curve25519-donna-c64 | head -123456 | tail -1

test-curve25519-donna: test-curve25519.c curve25519-donna.a
	gcc -o test-curve25519-donna test-curve25519.c curve25519-donna.a -Wall

test-curve25519-donna-x86-64: test-curve25519.c curve25519-donna-x86-64.a
	gcc -o test-curve25519-donna-x86-64 test-curve25519.c curve25519-donna-x86-64.a -Wall

test-curve25519-donna-c64: test-curve25519.c curve25519-donna-c64.a
	gcc -o test-curve25519-donna-c64 test-curve25519.c curve25519-donna-c64.a -Wall

speed-curve25519-donna: speed-curve25519.c curve25519-donna.a
	gcc -o speed-curve25519-donna speed-curve25519.c curve25519-donna.a -Wall

speed-curve25519-donna-x86-64: speed-curve25519.c curve25519-donna-x86-64.a
	gcc -o speed-curve25519-donna-x86-64 speed-curve25519.c curve25519-donna-x86-64.a -Wall

speed-curve25519-donna-c64: speed-curve25519.c curve25519-donna-c64.a
	gcc -o speed-curve25519-donna-c64 speed-curve25519.c curve25519-donna-c64.a -Wall

test-sc-curve25519-donna-x86-64: test-sc-curve25519.c curve25519-donna-x86-64.a
	gcc -o test-sc-curve25519-donna-x86-64 -O test-sc-curve25519.c curve25519-donna-x86-64.a test-sc-curve25519.s -Wall

test-sc-curve25519-donna-c64: test-sc-curve25519.c curve25519-donna-c64.a
	gcc -o test-sc-curve25519-donna-c64 -O test-sc-curve25519.c curve25519-donna-c64.a test-sc-curve25519.s -Wall
