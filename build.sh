#!/bin/bash
OUT_DIR=out/

CC=/media/ubuntu/4T/gcc/R/clang-r383902c/bin/clang
CROSS_COMPILE=/media/ubuntu/4T/gcc/R/aarch64-linux-android-4.9/bin/aarch64-linux-android-
CROSS_COMPILE_ARM32=/media/ubuntu/4T/gcc/R/arm-linux-androideabi-4.9/bin/arm-linux-androideabi-

make ARCH=arm64 \
	O=${OUT_DIR} \
	vendor/kona-perf_defconfig \
	-j4


cd ${OUT_DIR}

make ARCH=arm64 \
	O=${OUT_DIR} \
	CC=$CC \
	CLANG_TRIPLE="aarch64-linux-gnu-" \
	CROSS_COMPILE=$CROSS_COMPILE \
	CROSS_COMPILE_ARM32=$CROSS_COMPILE_ARM32 \
	-Werror \
	-j16
