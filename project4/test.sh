make
insmod romfs.ko hidden_file_name=aa encrypted_file_name=bb exec_file_name=cc
mount -o loop test.img /mnt -t romfs
ls -l /mnt
cat /mnt/bb
/mnt/cc
umount /mnt
rmmod romfs
