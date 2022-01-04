#!/bin/bash -xe

# Needs to happen outside Docker image, so we can reuse GitHub
# authentication. You can move this into the script if needed,
# but you'll have to figure out how to pass the right tokens.
#rm -rf build/models
#scripts/download_models.sh

# Run the rest of the script inside an Ubuntu 18.04 container,
# so that we get a glibc that's widely compatible.
sudo docker run -it -v`pwd`:/spchcat ubuntu:bionic /spchcat/scripts/generate_release_docker.sh