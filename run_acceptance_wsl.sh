#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT_DIR"

echo "[0/5] Checking tools"
for tool in gcc make grep wc; do
	if ! command -v "$tool" >/dev/null 2>&1; then
		echo "Missing required tool: $tool"
		echo "On Ubuntu WSL, run: sudo apt update && sudo apt install -y build-essential"
		exit 1
	fi
done

echo "[1/5] Cleaning previous generated files"
rm -f \
	test_sparse verify_sparse dump_sparse.c dump_sparse.o dump_sparse_accept.o \
	src/block.o src/datadesc.o src/plugins.o src/target_base.o \
	targets/target_c99.o \
	plugins/datacopy_foreach.o plugins/datacopy_121_reference.o \
	plugins/block2data_arithmetic.o plugins/block2data_sparse_const.o \
	main_test/test.sparse_const_basic.o main_test/test.sparse_const_verify.o \
	acceptance_run.log acceptance_verify.log

echo "[2/5] Running sparse-const generator and checking plugin hit"
make -f Makefile_test_sparse_const run 2>&1 | tee acceptance_run.log
grep -q "SPARSE PATH HIT" acceptance_run.log
test -s dump_sparse.c

echo "[3/5] Compiling generated dump_sparse.c"
gcc -c dump_sparse.c -o dump_sparse_accept.o

echo "[4/5] Running numerical verification"
make -f Makefile_test_sparse_const verify_sparse 2>&1 | tee acceptance_verify.log
./verify_sparse | tee -a acceptance_verify.log
grep -q "^OK$" acceptance_verify.log

echo "[5/5] Checking multiplication count"
M=3
mul_count="$(grep -oE 'var_[0-9]+_\[[^]]+\]=[^;]+\*' dump_sparse.c | wc -l | tr -d '[:space:]')"
echo "K statistics: Z=12 zero values, O=1 one value, M=3 non-zero non-one values"
echo "Generated multiplication assignment count: ${mul_count}"
if [ "$mul_count" -gt "$M" ]; then
	echo "FAIL: multiplication assignment count ${mul_count} > M ${M}"
	exit 1
fi

echo
echo "ACCEPTANCE SELF-CHECK PASSED"
echo "Expected evidence:"
echo "- SPARSE PATH HIT appears in acceptance_run.log"
echo "- dump_sparse.c was generated and compiled"
echo "- verify_sparse output is OK"
echo "- multiplication assignment count <= M"
