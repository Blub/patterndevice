#!/bin/bash

major=$(echo $BASH_VERSION | grep -o '^4[0-9]*')

if [ "$major" -lt 4 ] ; then
	echo "skip (not bash 4)"
	exit 0
fi

if [ ! -x ../binyes ] ; then
	echo "skip (binyes not built)"
	exit 0
fi

# BEGIN TEST:

# Start binyes
coproc byes ( ../binyes )

# Write the pattern
echo Correct Pattern >&${byes[1]}

# Close binyes stdin to tell it to start repeating
eval "exec ${byes[1]}>&-"

# Read a line from the pattern
read line <&${byes[0]}
read line2 <&${byes[0]}

# close binyes
eval "exec ${byes[0]}>&-"
kill $byes_PID

# CHECK
if [ "x$line" != "xCorrect Pattern" ] ; then
	echo "fail (incorrect line 1)"
	exit 1
fi

if [ "x$line2" != "xCorrect Pattern" ] ; then
	echo "fail (incorrect line 2)"
	exit 1
fi

echo pass
exit 0
