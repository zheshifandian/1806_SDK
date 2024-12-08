#!/bin/ash

set -x

export PATH=$PATH

PUBDIRPATH=/sf16a18/trace
[ ! -e $PUBDIRPATH ] && mkdir -p $PUBDIRPATH && chmod 1777 $PUBDIRPATH
#clear folder if exceed max to avoid storage crash
folder_cnt=0
du -d 1 $PUBDIRPATH | ( while read line
do
	if [ ${#line} -gt 25 ]; then
	folder_cnt=$(($folder_cnt + 1))
    fi
done
if [ $folder_cnt -gt 10 ]; then
	#clear old trace floder, Keep only the latest 10 floder
	old_trace_floder=`ls $PUBDIRPATH -lt | grep ^d | sed -n '11,100p' | awk '{print $9}'`
	for delete_floder in $old_trace_floder
	do
		rm -rf $PUBDIRPATH/$delete_floder
	done
fi
)

DIRNAME=$(date +"%F-%Hh%Mm%S")
DIRPATH=/tmp/$DIRNAME
[ -e $DIRPATH ] && mv $DIRPATH{,.old}
mkdir -p $DIRPATH
#rm -f $DIRPATH/../umh_dir

params_realm="nomail"
for param in $params_realm; do eval $param=false; done

for param in $(echo $1 | sed 's/,/ /g'); do
	[ "$param" = "nomail" ] && nomail=true
done


# Disable MAC Platform Clock gating before any access in the platform
# CLOCKGATESTAT=`mem 0xc09000e0`
#mem 0xc09000e0 w 0x00000010

#2 is stderr
exec 2> $DIRPATH/umh_exec_err
dmesg > $DIRPATH/dmesg

siwifi_diag=0

for i in /sys/kernel/debug/ieee80211/*
do
if [ -d $i ] && [ -f $i/siwifi/band_type ]; then
    if [ `cat $i/siwifi/band_type` = $1 ]; then
        siwifi_diag=$i/siwifi/diags
    fi
fi
done

echo $siwifi_diag

for i in $siwifi_diag/*; do cat $i > $DIRPATH/$(basename $i); done

echo "$(date +"%F-%Hh%Mm%S")--$1--` cat $siwifi_diag/error`" >> $PUBDIRPATH/error_list.txt
#check if log file is too large
logfile=`wc -c < $PUBDIRPATH/error_list.txt`
if [ $logfile -gt 1024000 ]; then
	rm $PUBDIRPATH/error_list.txt
fi

# Re-enable MAC Platform Clock gating
#mem 0xc09000e0 w 0x$CLOCKGATESTAT

msg="$(cat $DIRPATH/error | sed 's/\"/\\\"/')"
cp /lib/firmware/*.txt $DIRPATH

genvcd $DIRPATH
gendesc $DIRPATH
gendiags $DIRPATH


(
cd $DIRPATH/..
$nomail || tar zcf $DIRNAME.tgz $DIRNAME
#ln -sTf $DIRNAME umh_dir

cp -a $DIRNAME $PUBDIRPATH
chmod -R 777 $PUBDIRPATH/$DIRNAME
sync
$nomail || sendmail.py "$(hostname): $msg" "" $DIRNAME.tgz

rm -rf $DIRNAME.tgz
rm -rf $DIRNAME
)&
