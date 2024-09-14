#!/bin/sh
mkdir /tmp/mmctest
mkdir /tmp/mmctest_new
mount /dev/mmcblk0 /tmp/mmctest
i=1
while [ $i -le 10 ]
do
mv /tmp/mmctest/vmlinux-dasm /tmp/mmctest_new/
echo "IN test over."
md5sum /tmp/mmctest_new/vmlinux-dasm >> /tmp/mmctest/md5sum_new.txt
#cmp
cmp /tmp/mmctest/md5sum_new.txt /tmp/mmctest/md5sum.txt > /dev/zero
if [ $? -eq 1 ]; then
        echo "fail" >> /tmp/mmctest/log.txt
        echo "error! exit here"
        mv /tmp/mmctest_new/vmlinux-dasm /tmp/mmctest/
        umount /dev/mmcblk0
        break
else
        echo "success" >> /tmp/mmctest/log.txt
fi
mv /tmp/mmctest_new/vmlinux-dasm /tmp/mmctest/
echo "OUT test over"
j=5
while [ $j -ge 0 ]
do
        echo "$j..."
        sleep 1
        j=$(($j-1))
done
rm /tmp/mmctest/md5sum_new.txt
done
umount /dev/mmcblk0
#redo
