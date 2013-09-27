#!/bin/sh

set -e

autoreconf --force --install --symlink --warnings=all

args="\
--sysconfdir=/etc \
--localstatedir=/var \
--libdir=/usr/lib"

./configure CFLAGS='-g -O0' $args "$@"
make clean
