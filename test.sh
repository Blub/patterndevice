#!/bin/sh

if [ ! -e /dev/pattern ] ; then
	echo "/dev/pattern not found, please start it first"
	echo "Use ./patterndev -n pattern"
	echo "And make sure we have read/write access."
	exit 1
fi

exec 7<>/dev/pattern

echo It\'s working >&7

cat <&7 | dd if=/dev/stdin of=/dev/stdout bs=128 count=1
echo
exec 7>&-

echo "You should be seeing a bunch of lines above stating that it's working..."
echo "otherwise something's wrong"
