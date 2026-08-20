#!/bin/bash
. $(dirname "$0")/setpath.sh
obj/sp --cart $1
$XAS99 -R -b -q -L out.lst out.a99 -o cart.bin
$EMUL99 scripts/emul99.cfg cart_rom=cart.bin $2
