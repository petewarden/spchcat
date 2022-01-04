#!/bin/bash

cd /spchcat/

# Install system dependencies.
apt-get -qq update
apt-get -qq install -y sox libsox-dev libpulse-dev make gcc g++ wget curl sudo

# Fetch the binary libraries distributed by Coqui.
scripts/download_libs.sh

# Build the tool from source.
make clean && make spchcat LINK_PATH_STT=-Lbuild/lib EXTRA_CFLAGS_STT=-Ibuild/lib

# Package into a Debian/Ubuntu installer.
scripts/create_deb_package.sh