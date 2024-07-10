#!/bin/sh

# Check Installation supports this example
checkinstall.exe -p install.pkg --nobanner || exit

# Select CrossCompiler for OR1K/or1k
CROSS=RISCV32

# Build Application
make -C application CROSS=${CROSS}

# Build Platform
make -C platform


platform/platform.${IMPERAS_ARCH}.exe -program top.cpu1=application/program1.RISCV32.elf --program top.cpu2=application/program2.RISCV32.elf

# platform/platform.${IMPERAS_ARCH}.exe -program top.cpu1=application/program1.RISCV32.elf --program top.cpu2=application/program2.RISCV32.elf --mpdegui

