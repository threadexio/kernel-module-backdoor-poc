EXTRA_CFLAGS = -Wall -O2

obj-m := main.o

all:
	sed -i "s|^#define SYS_CALL_TABLE_ADDR.*$$|#define SYS_CALL_TABLE_ADDR $(shell echo "0x`sudo cat /proc/kallsyms |grep -E ' sys_call_table$$'`" | awk '{print $$1}')|g" main.c
	-make -C /lib/modules/$(shell uname -r)/build M=$$PWD modules
	-gcc -o exploit exploit.c

clean:
	-make -C /lib/modules/$(shell uname -r)/build M=$$PWD clean
	-rm exploit

restart:
	vagrant halt
	vagrant up
	vagrant ssh

enter:
	vagrant ssh