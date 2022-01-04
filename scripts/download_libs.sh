#!/bin/bash -xe

BUILD_DIR=build/

ARCHITECTURE=`uname -m`

if [[ ${ARCHITECTURE} = "armv7l" ]]
then
LIB_URL="https://github.com/coqui-ai/STT/releases/download/v1.1.0/native_client.tflite.linux.armv7.tar.xz"
else
LIB_URL="https://github.com/coqui-ai/STT/releases/download/v1.1.0/native_client.tflite.Linux.tar.xz"
fi
LIB_TMP_DIR=${BUILD_DIR}lib_tmp/
LIB_DIR=${BUILD_DIR}lib/

# Download, extract and copy the libraries from Coqui's binary release.
rm -rf ${LIB_TMP_DIR} && mkdir -p ${LIB_TMP_DIR}
wget -q ${LIB_URL} -O ${LIB_TMP_DIR}native_client.tflite.Linux.tar.xz
unxz ${LIB_TMP_DIR}native_client.tflite.Linux.tar.xz
rm -rf ${LIB_DIR}  && mkdir -p ${LIB_DIR}
tar -xf ${LIB_TMP_DIR}native_client.tflite.Linux.tar --directory ${LIB_DIR}
