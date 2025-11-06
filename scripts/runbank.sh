#!/bin/bash
obj/sp $1
rm cart_b*.bin cart.bin
~/ti99/xdt99/xas99.py -R -b -q -L out.lst out.a99 -o cart.bin
cat cart_b*.bin >>cart.bin
~/ti99/emul99/bin/emul99 scripts/emul99.cfg cart_rom=cart.bin $2
