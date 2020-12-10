#!/bin/bash

# Copyright © 2015-2016 Collabora Ltd.
# Copyright © 2020 Ralf Habacker <ralf.habacker@freenet.de>
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation files
# (the "Software"), to deal in the Software without restriction,
# including without limitation the rights to use, copy, modify, merge,
# publish, distribute, sublicense, and/or sell copies of the Software,
# and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
# BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
# ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

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

cmake -D CMAKE_BUILD_TYPE=RelWithDebInfo .. -DSQLCIPHER_LIBRARIES=/usr/lib64/libsqlcipher.so
${make}

# run x session
xvfb-run -s '+extension GLX +render' -a -n 99 openbox &
export DISPLAY=:99

# setup Qt plugin path
export QT_PLUGIN_PATH=$(pwd)/lib:${QT_PLUGIN_PATH:-.}

# fix not finding created sqldrivers
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
killall -s 9 xvfb kdeinit5 openbox dbus-daemon || true
# vim:set sw=4 sts=4 et:
