#! /bin/bash

set -e
set -u
set -x

test -n "$QTHREADS_NAME" || exit 1
test -n "$QTHREADS_DIR" || exit 1
test -n "$MAKE" || exit 1

BASEDIR="$(pwd)"

rm -rf "$QTHREADS_DIR"
tar xzf "$QTHREADS_NAME.tar.bz2"

rm -rf "$QTHREADS_DIR-build"
mkdir "$QTHREADS_DIR-build"
pushd "$QTHREADS_DIR-build"
"$BASEDIR/$QTHREADS_DIR/configure" --prefix="$BASEDIR/$QTHREADS_DIR-install" \
                                   --enable-guard-pages --enable-debug=yes
"$MAKE"
"$MAKE" install

popd

# rm -rf "$QTHREADS_DIR"
# rm -rf "$QTHREADS_DIR-build"
