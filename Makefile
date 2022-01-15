EXTRA_CFLAGS = -Wall -O2

obj-m := backdoor.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$$PWD modules
	gcc -o exploit exploit.c

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$$PWD clean
	rm exploit