#!/bin/bash
make
insmod watch.ko
sysbench --test=fileio --num-threads=16 --file-total-size=5G --file-test-mode=rndrw prepare
./test_fileio
sysbench --test=fileio --num-threads=20 --file-total-size=5G --file-test-mode=rndrw cleanup
mv ./watch_result ./watch_fileio
./test_thread
mv ./watch_result ./watch_thread
./test_mutex
mv ./watch_result ./watch_mutex
rmmod watch.ko
make clean