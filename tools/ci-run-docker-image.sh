#!/bin/sh
#
# @author Ralf Habacker <ralf.habacker@freenet.de>
#
: "${image:=opensuse/leap:15.6}"

# enter source dir
# cd ~/src/kmymoney

case "$ci_distro" in
    (opensuse-leap)
        image=opensuse/leap:15.6
        ;;
    (opensuse-tumbleweed)
        image=opensuse/tumbleweed
        ;;
    (ubuntu)
        image=ubuntu:20.10
        ;;
esac

#inside docker run
cat << EOF
#inside docker run the following commands

cd /mnt

tools/ci-install.sh
tools/ci-build.sh
EOF

options=
if [ "$1" == "--use-host-display" ]; then
    options="-v $HOME/.Xauthority:/root/.Xauthority:rw --env=DISPLAY --net=host"
    shopts="export DISPLAY=$DISPLAY"
fi

sudo docker pull $image
sudo docker run \
    -v $PWD:/mnt \
    $options \
    -it $image \
    /bin/bash -c "cd /mnt; $shopts export ci_distro=$ci_distro; export ci_variant=$ci_variant; tools/ci-install.sh; tools/ci-build.sh; bash"

