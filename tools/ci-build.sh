#!/bin/bash

# SPDX-FileCopyrightText: 2015-2016 Collabora Ltd.
# SPDX-FileCopyrightText: 2020-2024 Ralf Habacker ralf.habacker @freenet.de
#
# SPDX-License-Identifier: MIT

# fails also on piped failure
set -euo pipefail

# enable trace
set -x

NULL=

# ci_distro:
# OS distribution in which we are testing
# Typical values: ubuntu, debian; maybe fedora in future
: "${ci_distro:=opensuse}"

# ci_docker:
# If non-empty, this is the name of a Docker image. ci-install.sh will
# fetch it with "docker pull" and use it as a base for a new Docker image
# named "ci-image" in which we will do our testing.
#
# If empty, we test on "bare metal".
# Typical values: ubuntu:xenial, debian:jessie-slim
: "${ci_docker:=}"

# ci_host:
# See ci-install.sh
: "${ci_host:=native}"

# ci_parallel:
# A number of parallel jobs, passed to make -j
: "${ci_parallel:=1}"

# ci_sudo:
# If yes, assume we can get root using sudo; if no, only use current user
: "${ci_sudo:=no}"

# ci_test:
# If yes, run tests; if no, just build
: "${ci_test:=yes}"

# ci_test_fatal:
# If yes, test failures break the build; if no, they are reported but ignored
: "${ci_test_fatal:=yes}"

# ci_variant:
# One of debug, reduced, legacy, production
: "${ci_variant:=production}"

if [ -n "$ci_docker" ]; then
    exec docker run \
        --env=ci_docker="" \
        --env=ci_host="${ci_host}" \
        --env=ci_parallel="${ci_parallel}" \
        --env=ci_sudo=yes \
        --env=ci_test="${ci_test}" \
        --env=ci_test_fatal="${ci_test_fatal}" \
        --env=ci_variant="${ci_variant}" \
        --privileged \
        ci-image \
        tools/ci-build.sh
fi

maybe_fail_tests () {
    if [ "$ci_test_fatal" = yes ]; then
        exit 1
    fi
}

srcdir="$(pwd)"
rm -rf ci-build-${ci_variant}-${ci_host}
mkdir -p ci-build-${ci_variant}-${ci_host}
cd ci-build-${ci_variant}-${ci_host}

# exceeds KDE CI job output limit
#make="make -j${ci_parallel} V=1 VERBOSE=1"
make="make -j${ci_parallel}"

# kmymoney specific command line
case $ci_variant in
    (kf6*)
        cmake -D CMAKE_BUILD_TYPE=RelWithDebInfo .. -DBUILD_WITH_QT6=1 -DSQLCIPHER_LIBRARIES=/usr/lib64/libsqlcipher.so
        ;;
    (*)
        cmake -D CMAKE_BUILD_TYPE=RelWithDebInfo .. -DSQLCIPHER_LIBRARIES=/usr/lib64/libsqlcipher.so
        ;;
esac

${make}

# start x session if not present
if [ -z "$DISPLAY" ]; then
    xvfb-run -s '+extension GLX +render' -a -n 99 openbox &
    export DISPLAY=:99
    own_session=1
fi

# setup Qt plugin path
export QT_PLUGIN_PATH=$(pwd)/lib:${QT_PLUGIN_PATH:-.}

# fix not finding created sqldrivers (kmymoney specific)
# qt expects a subdir associated with the plugin type
ln -s . lib/sqldrivers

# start KDE session
kdeinit5

# run tests
[ "$ci_test" = no ] || ctest -VV -E reports-chart-test || maybe_fail_tests

# install files
${make} install DESTDIR=$(pwd)/DESTDIR

# list files
( cd DESTDIR && find . -ls)

# kill background processes
if [ "$own_session" -eq 1 }; then
    killall -s 9 xvfb kdeinit5 openbox dbus-daemon || true
fi
