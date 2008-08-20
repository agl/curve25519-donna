targets: curve25519-donna.a curve25519-donna-x86-64.a

test: test-donna test-donna-x86-64

clean:
	rm -f *.o *.a *.pp test-curve255-donna test-curve25519-donna-x86-64 speed-curve25519-donna speed-curve25519-donna-x86-64

curve25519-donna.a: curve25519-donna.o
	ar -rc curve25519-donna.a curve25519-donna.o

curve25519-donna.o: curve25519-donna.c
	gcc -O2 -c curve25519-donna.c

curve25519-donna-x86-64.a: curve25519-donna-x86-64.o curve25519-donna-x86-64.s.o
	ar -rc curve25519-donna-x86-64.a curve25519-donna-x86-64.o curve25519-donna-x86-64.s.o

curve25519-donna-x86-64.s.o: curve25519-donna-x86-64.s.pp
	as -o curve25519-donna-x86-64.s.o curve25519-donna-x86-64.s.pp

curve25519-donna-x86-64.s.pp: curve25519-donna-x86-64.s
	cpp curve25519-donna-x86-64.s > curve25519-donna-x86-64.s.pp

curve25519-donna-x86-64.o: curve25519-donna-x86-64.c
	gcc -O2 -c curve25519-donna-x86-64.c

test-donna: test-curve25519-donna
	./test-curve25519-donna | head -123456 | tail -1

test-donna-x86-64: test-curve25519-donna-x86-64
	./test-curve25519-donna-x86-64 | head -123456 | tail -1

test-curve25519-donna: test-curve25519.c curve25519-donna.a
	gcc -o test-curve25519-donna test-curve25519.c curve25519-donna.a

test-curve25519-donna-x86-64: test-curve25519.c curve25519-donna-x86-64.a
	gcc -o test-curve25519-donna-x86-64 test-curve25519.c curve25519-donna-x86-64.a

speed-curve25519-donna: speed-curve25519.c curve25519-donna.a
	gcc -o speed-curve25519-donna speed-curve25519.c curve25519-donna.a

speed-curve25519-donna-x86-64: speed-curve25519.c curve25519-donna-x86-64.a
	gcc -o speed-curve25519-donna-x86-64 speed-curve25519.c curve25519-donna-x86-64.a
