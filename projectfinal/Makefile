obj-m += watch.o
all: test_fileio test_mutex test_thread watch
test_thread: test.c
	gcc -Werror -Wall -DTHREAD -o test_thread test.c
test_mutex: test.c
	gcc -Werror -Wall -DMUTEX -o test_mutex test.c
test_fileio: test.c
	gcc -Werror -Wall -DFILEIO -o test_fileio test.c
watch:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm test_*