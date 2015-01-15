
all: as_hello c_hello_world dummy_test segfault_test

as_hello: as_hello_world.o
	ld -o ../bin/as_hello as_hello_world.o
	rm as_hello_world.o

as_hello_world.o: as_hello_world.S
	gcc -c as_hello_world.S

c_hello_world: c_hello_world.c
	gcc -o ../bin/c_hello c_hello_world.c

dummy_test: dummy_test.c
	gcc -o ../bin/dummy_test dummy_test.c

segfault_test: segfault_test.c
	gcc -o ../bin/segfault_test segfault_test.c