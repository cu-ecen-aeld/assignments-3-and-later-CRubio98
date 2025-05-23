#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.15.163
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR} || (echo "Directory: ${OUTDIR} could not be created"; exit 1;)

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

    make clean
    # Clean the kernel build tree
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper
    #Generate the config for our dev board
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
    # Build the kernel image
    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all
    # Build the device tree
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs
fi

echo "Adding the Image in outdir"
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}/

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# Creating rootfs directory
mkdir -p ${OUTDIR}/rootfs
cd ${OUTDIR}/rootfs

# Creating needed directories under rootfs
mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir -p usr/bin usr/lib  usr/sbin
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

# Make and install busybox in our roofs directory
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install

echo "Library dependencies"
cd ${OUTDIR}/rootfs
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

# Obtaining Sysroot path
SYSROOT=$(${CROSS_COMPILE}gcc -print-sysroot)

# Add library dependencies to rootfs
cp ${SYSROOT}/lib/ld-linux-aarch64.so.1 ${OUTDIR}/rootfs/lib
cp ${SYSROOT}/lib64/libm.so.6 ${OUTDIR}/rootfs/lib64
cp ${SYSROOT}/lib64/libc.so.6 ${OUTDIR}/rootfs/lib64
cp ${SYSROOT}/lib64/libresolv.so.2 ${OUTDIR}/rootfs/lib64


# Make device nodes: minimal rootFS only needs null and console
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 600 dev/console c 5 1

# Clean and build the writer utility using cross compiling
cd ${FINDER_APP_DIR}
make clean
make CROSS_COMPILE=${CROSS_COMPILE}


# Copy the finder related scripts and executables to the /home directory
# on the target rootfs
cp finder-test.sh finder.sh ${OUTDIR}/rootfs/home
cp -rf conf/ ${OUTDIR}/rootfs/home

# Copying the writer app crosscompiled
cp writer ${OUTDIR}/rootfs/home

# Copying autorun-qemu.sh into /home in the rootfs
cp autorun-qemu.sh ${OUTDIR}/rootfs/home

# Chown the root directory
cd ${OUTDIR}/rootfs
sudo chown -R root:root *

# Create initramfs.cpio.gz
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
cd ${OUTDIR}
gzip -f initramfs.cpio
