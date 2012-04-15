#!/bin/sh

passed=0
failed=0
skipped=0

check() {
	what="$1"
	shift
	result="$*"
	case "$result" in
		skip*)
			echo "[SKIP] $what: ${result##skip }"
			let skipped++
			;;
		fail*)
			echo "[FAIL] $what: ${result##fail }"
			let failed++
			;;
		pass*)
			echo "[PASS] $what"
			let passed++
			;;
	esac
}

cd tests
check binyes-bash4-coproc $(bash ./binyes-bash4.sh)
check binyes-fifo-any $(sh ./binyes-fifo.sh)

echo "Results:"
echo "  passed:  ${passed}"
echo "  skipped: ${skipped}"
echo "  failed:  ${failed}"

if [ "$failed" -ne 0 ] ; then
	exit 1
fi
exit 0
