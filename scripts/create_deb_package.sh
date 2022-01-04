#!/bin/bash -xe

NAME=spchcat
VERSION=0.0-2
ARCH=amd64
MAINTAINER=pete@petewarden.com
DESCRIPTION="Speech recognition tool to convert audio to text transcripts."

BUILD_DIR=build/
MODELS_DIR=${BUILD_DIR}models/
LIB_URL="https://github.com/coqui-ai/STT/releases/download/v1.1.0/native_client.tflite.Linux.tar.xz"
LIB_TMP_DIR=${BUILD_DIR}lib_tmp/
LIB_DIR=${BUILD_DIR}lib/
DEB_DIR=${BUILD_DIR}${NAME}_${VERSION}_${ARCH}/
DEB_MODELS_DIR=${DEB_DIR}/etc/spchcat/models/
DEB_LIB_DIR=${DEB_DIR}/usr/lib/
DEB_BIN_DIR=${DEB_DIR}/usr/local/bin/
DEB_DEBIAN_DIR=${DEB_DIR}DEBIAN/
DEB_CONTROL_FILE=${DEB_DEBIAN_DIR}control

rm -rf ${DEB_DIR}

# Copy over the language models. You'll need to run the download_releases.py
# first to fetch the required files.
mkdir -p ${DEB_MODELS_DIR}
cp -r --verbose ${MODELS_DIR}* ${DEB_MODELS_DIR}
# Remove the TensorFlow model graphs, since these are only used for training
# and are quite large.
find ${DEB_MODELS_DIR} -type f -name "*.pb*" -delete
# Some scorers are also very large, so for convenience remove them too.
find ${DEB_MODELS_DIR} -iname "*.scorer" -size +150M -delete

# Fetch the binary library release.
scripts/download_libs.sh
mkdir -p ${DEB_LIB_DIR}
cp -r  --verbose ${LIB_DIR}*.so* ${DEB_LIB_DIR}

# Copy over the binary executable.
mkdir -p ${DEB_BIN_DIR}
cp -r --verbose ${NAME} ${DEB_BIN_DIR}

# Set up the metadata for the package.
mkdir -p ${DEB_DEBIAN_DIR}
cat << EOM > ${DEB_CONTROL_FILE}
Package: ${NAME}
Version: ${VERSION}
Architecture: ${ARCH}
Maintainer: ${MAINTAINER}
Description: ${DESCRIPTION}
EOM

# Build the package.
dpkg-deb --build --root-owner-group ${DEB_DIR}