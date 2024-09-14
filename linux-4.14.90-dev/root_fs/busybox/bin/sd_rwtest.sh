#!/bin/sh
mkdir /tmp/sdmsg
mkdir /tmp/new
mount /dev/mmcblk0 /tmp/sdmsg -t ext4
i=1
while [ $i -le 10 ]
do
mv /tmp/sdmsg/vmlinux-dasm /tmp/new/
echo "IN test over."
md5sum /tmp/new/vmlinux-dasm >> /tmp/sdmsg/md5sum_new.txt
#cmp
cmp /tmp/sdmsg/md5sum_new.txt /tmp/sdmsg/md5sum.txt > /dev/zero
if [ $? -eq 1 ]; then
	echo "fail" >> /tmp/sdmsg/log.txt
	echo "error! exit here"
	mv /tmp/new/vmlinux-dasm /tmp/sdmsg/
	umount /dev/mmcblk0
	break
else
	echo "success" >> /tmp/sdmsg/log.txt
fi
mv /tmp/new/vmlinux-dasm /tmp/sdmsg/
echo "OUT test over"
j=5
while [ $j -ge 0 ]
do
	echo "$j..."
	sleep 1
	j=$(($j-1))
done
rm /tmp/sdmsg/md5sum_new.txt
done
umount /dev/mmcblk0
#redo
