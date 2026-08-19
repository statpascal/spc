#!/bin/bash
. $(dirname "$0")/setpath.sh
obj/sp --ea5 $1
rm -rf out.ea5
$XAS99 -R -i -q -L out.lst out.a99 -o out.ea5
$EMUL99 scripts/emul99.cfg cart_groms=:/modules/Ed-AssmG.Bin
