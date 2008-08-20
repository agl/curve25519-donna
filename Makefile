targets: curve25519-donna.a curve25519-donna-x86-64.a

clean:
	rm *.o *.a *.pp

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
