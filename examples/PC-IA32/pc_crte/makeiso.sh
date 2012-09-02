#!/bin/sh

# check to see if the correct tools are installed
for X in wc genisoimage
do
	if [ "$(which $X)" = "" ]; then
		echo "makeiso.sh error: $X is not in your path." >&2
		exit 1
	elif [ ! -x $(which $X) ]; then
		echo "makeiso.sh error: $X is not executable." >&2
		exit 1
	fi 
done

#check to see if standalone.bin is present
if [ ! -w standalone.bin ]; then 
	echo "makeiso.sh error: cannot find standalone.bin, did you compile it?" >&2 
	exit 1
fi

# enlarge the size of standalone.bin
SIZE=$(wc -c standalone.bin | awk '{print $1}')
FILL=$((1474560 - $SIZE))
dd if=/dev/zero of=fill.tmp bs=$FILL count=1 >/dev/null 2>&1
cat standalone.bin fill.tmp >standalone.img
rm -f fill.tmp

mkdir "cd"
mkdir "cd/boot"
mv standalone.img cd/boot
cd cd
genisoimage -b boot/standalone.img -c boot/boot.catalog -o ../standalone.iso .
cd ..
rm -rf cd
