#!/usr/bin/env bash
qemu-system-i386 -drive file=os.img,format=raw,if=floppy &
QEMU_PID=$!
START=$(date +%s%N)
while ! xdotool search --name "QEMU" 2>/dev/null; do
    sleep 0.005
done
END=$(date +%s%N)
ELAPSED=$(( (END - START) / 1000000 ))
kill $QEMU_PID 2>/dev/null
wait $QEMU_PID 2>/dev/null
echo "$ELAPSED"
