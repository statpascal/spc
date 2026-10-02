#!/bin/bash

set -e 

. $(dirname "$0")/setpath.sh
obj/sp $1
rm -f cart_b*.bin cart.bin
$XAS99 -R -b -q -L out.lst out.a99 -o cart.bin
cat cart_b*.bin >>cart.bin

if [ -f cart.bin ]; then
    $EMUL99 scripts/emul99.cfg cart_rom=cart.bin $2
fi

