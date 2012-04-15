#!/bin/sh

if [ ! -x ../binyes ] ; then
	echo "skip (binyes not built)"
	exit 0
fi

if [ ! -x $(which mkfifo) ] ; then
	echo "skip (mkfifo not found)"
	exit 0
fi

# find an external echo command because the shell's builtins
# generally don't seem to guarantee an error status on some systems
if [ ! -x $(which echo) ] ; then
	echo "skip (no extern echo command)"
	exit 0
fi
ECHO=$(which echo)

rm -f test_fifo

if ! mkfifo test_fifo ; then
	echo "fail (mkfifo failed)"
	exit 1
fi

# A helper to print a message and exit
die() {
	echo "$*"
	exit 1
}

# Open binyes' read end and pipe to a subshell
../binyes < test_fifo | {
	# Write to binyes
	$ECHO "Correct Pattern" > test_fifo || die "fail (failed to write to fifo)"
	
	read line
	read line2
	rm -f test_fifo
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
}

# line and line2 are NOT available here, they're part of the
# SUBshell only!
# this is what makes fifos inconvenient if you cannot use
# exec <... constructs
