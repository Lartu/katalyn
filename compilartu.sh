#!/bin/sh

set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
install_prefix=${PREFIX:-"$HOME"}

make -C "$script_dir"
make -C "$script_dir" install PREFIX="$install_prefix"

"$install_prefix/bin/kat" -n -a 'print("Katalyn compilation successful!");'
