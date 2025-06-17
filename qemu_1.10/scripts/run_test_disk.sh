#!/bin/sh
set -e

# Simple helper to create a qcow2 disk image and boot QEMU with it.
# Usage: ./scripts/run_test_disk.sh [image] [size]
# Default image name is 'disk.img' and size is 64M.
IMG=${1:-disk.img}
SIZE=${2:-64M}

# Create disk image if it doesn't already exist
if [ ! -f "$IMG" ]; then
    echo "Creating $IMG of size $SIZE"
    qemu-img create -f qcow2 "$IMG" "$SIZE"
fi

cd qemu-0.10.0
exec ./i386-softmmu/qemu -hda "../$IMG" -m 64
