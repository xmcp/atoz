#!/bin/bash
[ -f /tmp/hostshare/out.bin ] && rm /tmp/hostshare/out.bin
riscv32-unknown-linux-gnu-gcc /tmp/hostshare/out.S -o /tmp/hostshare/out.bin -L/root -lsysy -static
qemu-riscv32-static /tmp/hostshare/out.bin < /tmp/hostshare/input.txt