#!/bin/bash
# Measures wall-clock time until the kernel panic marker appears
BIN="$1"
OUT="$2"
rm -f "$OUT"
start=$(date +%s.%N)
"$BIN" --bios /home/michen/dev/riscv-programs/fw_jump.bin --kernel /home/michen/dev/riscv-programs/Image > "$OUT" 2>&1 &
PID=$!
# poll for the panic marker
for i in $(seq 1 1500); do
    if grep -q "Kernel panic" "$OUT" 2>/dev/null; then
        break
    fi
    if ! kill -0 $PID 2>/dev/null; then
        break
    fi
    sleep 0.02
done
end=$(date +%s.%N)
kill $PID 2>/dev/null
wait $PID 2>/dev/null
echo "$(echo "$end - $start" | bc)"