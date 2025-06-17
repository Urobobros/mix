#!/bin/sh
set -e

# Simple helper script to install SDL 1.2 development files on
# Debian/Ubuntu systems. Useful when building QEMU from a fresh
# environment without persistent package caches.

PKG="libsdl1.2-dev"

if dpkg -s "$PKG" >/dev/null 2>&1; then
    echo "$PKG already installed"
else
    echo "Installing $PKG via apt-get..."
    sudo apt-get update
    sudo apt-get install -y "$PKG"
fi
