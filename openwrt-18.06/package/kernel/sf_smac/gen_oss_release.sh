#!/bin/bash
cd $(dirname "$0")
find config -name '*.pdf' | xargs rm
rm src/bb_src/umac/fullmac/siwifi_repeater*

find src -name '*.c' -o -name '*.h' | while read i
do
	gcc -fpreprocessed -dD -E -P $i -o tmp || exit 1
	sed -i '/^MODULE_AUTHOR/d' tmp
	mv tmp $i
done

sed -i '/SIWIFI_REPEATER/d' Makefile
rm gen_oss_release.sh
