#!/bin/sh
#
# @author Ralf Habacker <ralf.habacker@freenet.de>
#

# enter source dir
# cd ~/src/kmymoney

#inside docker run
cat << EOF
#inside docker run the following commands

cd /mnt

tools/ci-install.sh
tools/ci-build.sh
EOF

sudo docker pull opensuse/leap
sudo docker run -v $PWD:/mnt -it opensuse/leap /bin/bash

