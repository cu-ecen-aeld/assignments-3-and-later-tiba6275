#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.
# Modified by Tim Bailey for CU Boulder ECEN 5713 Assignment 3
# Referenced lecture slides and https://subscription.packtpub.com/book/iot-and-hardware/9
# 781787124202/1/ch01lvl1sec06/5-the-linux-root-filesystemld-2.31. for libraries

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$(realpath $1)
	echo "Using passed directory ${OUTDIR} for output"
fi

if [ ! -d "$OUTDIR" ]; then
    mkdir -p ${OUTDIR} || { echo "Directory could not be created."; exit 1; }
fi

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # Deep clean kernel build tree
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper
    
    # Configure for virt arm dev board
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig

    # Build kernal image for booting in QEMU
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all

    # Build kernel modules
    # make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules

    # Build devicetree
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs
fi

echo "Adding the Image in outdir"
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}
echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# Create necessary base directories
mkdir ${OUTDIR}/rootfs
cd ${OUTDIR}/rootfs
mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir -p usr/bin usr/lib usr/sbin
mkdir -p var/log

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # Configure busybox
    make distclean
    make defconfig
else
    cd busybox
fi

# Make and install busybox
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install
cd ${OUTDIR}/rootfs
echo "Library dependencies"
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

# Add library dependencies to rootfs
# Reference: https://subscription.packtpub.com/book/iot-and-hardware/9
# 781787124202/1/ch01lvl1sec06/5-the-linux-root-filesystemld-2.31.
SYSROOT=$(${CROSS_COMPILE}gcc -print-sysroot)
# Interpreter
cp -a $SYSROOT/lib/ld-linux-aarch64.so.1 lib
# Libraries
cp -a $SYSROOT/lib64/libm.so.6 lib64
cp -a $SYSROOT/lib64/libresolv.so.2 lib64
cp -a $SYSROOT/lib64/libc.so.6 lib64
cp -a $SYSROOT/lib64/libm-2.31.so lib64
cp -a $SYSROOT/lib64/libresolv-2.31.so lib64
cp -a $SYSROOT/lib64/libc-2.31.so lib64
cp -a $SYSROOT/lib64/ld-2.31.so lib64

# Make device nodes
sudo mknod -m 777 dev/null c 1 3
sudo mknod -m 777 dev/console c 5 1

# Clean and build the writer utility
cd ${FINDER_APP_DIR}
make clean
make CROSS_COMPILE=${CROSS_COMPILE}

# Copy the finder related scripts and executables to the /home directory
# on the target rootfs
cp -r ${FINDER_APP_DIR}/conf/ ${OUTDIR}/rootfs/home
cp -r ${FINDER_APP_DIR}/conf/ ${OUTDIR}/rootfs/
cp ${FINDER_APP_DIR}/autorun-qemu.sh ${OUTDIR}/rootfs/home
cp ${FINDER_APP_DIR}/dependencies.sh ${OUTDIR}/rootfs/home
cp ${FINDER_APP_DIR}/finder.sh ${OUTDIR}/rootfs/home
cp ${FINDER_APP_DIR}/finder-test.sh ${OUTDIR}/rootfs/home
cp ${FINDER_APP_DIR}/start-qemu-app.sh ${OUTDIR}/rootfs/home
cp ${FINDER_APP_DIR}/start-qemu-terminal.sh ${OUTDIR}/rootfs/home
cp ${FINDER_APP_DIR}/writer ${OUTDIR}/rootfs/home
cp ${FINDER_APP_DIR}/writer.c ${OUTDIR}/rootfs/home
cp ${FINDER_APP_DIR}/writer.sh ${OUTDIR}/rootfs/home
cp ${FINDER_APP_DIR}/Makefile ${OUTDIR}/rootfs/home

# Chown the root directory
cd ${OUTDIR}/rootfs/
sudo chown -R root:root *

# Create initramfs.cpio.gz
cd "$OUTDIR/rootfs"
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio && cd ..
gzip -f initramfs.cpio
